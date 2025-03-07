//! Defines Func, SignatureBuilder, and Signature structs.
use crate::{
    error::HostFuncError, io::WasmValTypeList, CallingFrame, Engine, FuncType, ValType,
    WasmEdgeResult, WasmValue,
};
use wasmedge_sys as sys;

/// Defines a host function instance.
///
/// A WasmEdge [Func] is a wasm function instance, which is a "wrapper" of the original function (defined in either the host or the WebAssembly module) over the runtime [module instance](crate::Instance) of its originating [module](crate::Module).
///
/// # Example
///
/// The following example shows how to create a host function, and invoke it with a given executor.
///
/// ```rust
/// // If the version of rust used is less than v1.63,
/// // #![feature(explicit_generic_args_with_impl_trait)]
///
/// use wasmedge_sdk::{Func, Executor, params, WasmVal, error::HostFuncError, WasmValue, ValType, CallingFrame};
///
/// // A native function to be wrapped as a host function
/// fn real_add(_: &CallingFrame, input: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
///     if input.len() != 2 {
///         return Err(HostFuncError::User(1));
///     }
///
///     let a = if input[0].ty() == ValType::I32 {
///         input[0].to_i32()
///     } else {
///         return Err(HostFuncError::User(2));
///     };
///
///     let b = if input[1].ty() == ValType::I32 {
///         input[1].to_i32()
///     } else {
///         return Err(HostFuncError::User(3));
///     };
///
///     let c = a + b;
///     Ok(vec![WasmValue::from_i32(c)])
/// }
///
/// // create a host function
/// let result = Func::wrap::<(i32, i32), i32>(real_add);
/// assert!(result.is_ok());
/// let func = result.unwrap();
///
/// // create an executor
/// let mut executor = Executor::new(None, None).unwrap();
///
/// // call the host function
/// let result = func.call(&mut executor, params!(2, 3));
/// assert!(result.is_ok());
/// let returns = result.unwrap();
/// assert_eq!(returns[0].to_i32(), 5);
/// ```
/// [[Click for more examples]](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge-sdk/examples)
///
#[derive(Debug)]
pub struct Func {
    pub(crate) inner: sys::Function,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Func {
    /// Creates a host function of the given func type.
    ///
    /// N.B. that this function can be used in thread-safe scenarios.
    ///
    /// # Arguments
    ///
    /// * `ty` - The function type.
    ///
    /// * `real_func` - The native function that will be wrapped as a host function.
    ///
    /// # Error
    ///
    /// If fail to create the host function, then an error is returned.
    pub fn new(
        ty: FuncType,
        real_func: impl Fn(&CallingFrame, Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError>
            + Send
            + Sync
            + 'static,
    ) -> WasmEdgeResult<Self> {
        let boxed_func = Box::new(real_func);
        let inner = sys::Function::create(&ty.into(), boxed_func, 0)?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

    /// Creates a host function by wrapping a native function.
    ///
    /// N.B. that this function can be used in thread-safe scenarios.
    ///
    /// # Arguments
    ///
    /// * `real_func` - The native function to be wrapped.
    ///
    /// # Error
    ///
    /// If fail to create the host function, then an error is returned.
    pub fn wrap<Args, Rets>(
        real_func: impl Fn(&CallingFrame, Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError>
            + Send
            + Sync
            + 'static,
    ) -> WasmEdgeResult<Self>
    where
        Args: WasmValTypeList,
        Rets: WasmValTypeList,
    {
        let boxed_func = Box::new(real_func);
        let args = Args::wasm_types();
        let returns = Rets::wasm_types();
        let ty = FuncType::new(Some(args.to_vec()), Some(returns.to_vec()));
        let inner = sys::Function::create(&ty.into(), boxed_func, 0)?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

    /// Creates a host function by wrapping a native function.
    ///
    /// N.B. that this function is used for single-threaded scenarios. If you would like to use hostfunc call chaining design, you should use this method to create a [Func] instance.
    ///
    /// # Arguments
    ///
    /// * `real_func` - The native function to be wrapped.
    ///
    /// # Error
    ///
    /// If fail to create the host function, then an error is returned.
    pub fn wrap_single_thread<Args, Rets>(
        real_func: impl Fn(&CallingFrame, Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError>
            + 'static,
    ) -> WasmEdgeResult<Self>
    where
        Args: WasmValTypeList,
        Rets: WasmValTypeList,
    {
        let boxed_func = Box::new(real_func);
        let args = Args::wasm_types();
        let returns = Rets::wasm_types();
        let ty = FuncType::new(Some(args.to_vec()), Some(returns.to_vec()));
        let inner = sys::Function::create_single_thread(&ty.into(), boxed_func, 0)?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

    /// Returns the exported name of this function.
    ///
    /// Notice that this field is meaningful only if this host function is used as an exported instance.
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    /// Returns the name of the [module instance](crate::Instance) from which this function exports.
    ///
    /// Notice that this field is meaningful only if this host function is used as an exported instance.
    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    /// Returns the type of the host function.
    ///
    /// If fail to get the signature, then an error is returned.
    pub fn ty(&self) -> WasmEdgeResult<FuncType> {
        let func_ty = self.inner.ty()?;
        Ok(func_ty.into())
    }

    /// Returns a reference to this function instance.
    pub fn as_ref(&self) -> FuncRef {
        let inner = self.inner.as_ref();
        FuncRef { inner }
    }

    /// Runs this host function and returns the result.
    ///
    /// # Arguments
    ///
    /// * `engine` - The object implementing the [Engine](crate::Engine) trait.
    ///
    /// * `args` - The arguments passed to the host function.
    ///
    /// # Error
    ///
    /// If fail to run the host function, then an error is returned.
    ///
    pub fn call<E: Engine>(
        &self,
        engine: &mut E,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        engine.run_func(self, args)
    }
}

/// Defines a type builder for creating a [FuncType](https://wasmedge.github.io/WasmEdge/wasmedge_types/struct.FuncType.html) instance.
#[derive(Debug, Default)]
pub struct FuncTypeBuilder {
    args: Option<Vec<ValType>>,
    returns: Option<Vec<ValType>>,
}
impl FuncTypeBuilder {
    /// Creates a new [FuncTypeBuilder].
    pub fn new() -> Self {
        Self {
            args: None,
            returns: None,
        }
    }

    /// Adds arguments to the function type.
    ///
    /// # Argument
    ///
    /// `args` specifies the arguments to be added to the function type.
    pub fn with_args(self, args: impl IntoIterator<Item = ValType>) -> Self {
        Self {
            args: Some(args.into_iter().collect::<Vec<_>>()),
            returns: self.returns,
        }
    }

    /// Adds a single argument to the function type.
    ///
    /// # Argument
    ///
    /// `arg` specifies the argument to be added to the function type.
    pub fn with_arg(self, arg: ValType) -> Self {
        self.with_args(std::iter::once(arg))
    }

    /// Adds returns to the function type.
    ///
    /// # Argument
    ///
    /// `returns` specifies the returns to be added to the function type.
    pub fn with_returns(self, returns: impl IntoIterator<Item = ValType>) -> Self {
        Self {
            args: self.args,
            returns: Some(returns.into_iter().collect::<Vec<_>>()),
        }
    }

    /// Adds a single return to the function type.
    ///
    /// # Argument
    ///
    /// `ret` specifies the return to be added to the function type.
    pub fn with_return(self, ret: ValType) -> Self {
        self.with_returns(std::iter::once(ret))
    }

    /// Returns a function type.
    pub fn build(self) -> FuncType {
        FuncType::new(self.args, self.returns)
    }
}

/// Defines a reference to a [host function](crate::Func).
///
/// The [table_and_funcref](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge-sdk/examples/table_and_funcref.rs) example presents how to obtain and use [FuncRef].
#[derive(Debug, Clone)]
pub struct FuncRef {
    pub(crate) inner: sys::FuncRef,
}
impl FuncRef {
    /// Returns the underlying wasm type of the host function this [FuncRef] points to.
    ///
    /// # Errors
    ///
    /// If fail to get the function type, then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<FuncType> {
        let ty = self.inner.ty()?;
        Ok(ty.into())
    }

    /// Runs this host function the reference refers to.
    ///
    /// # Arguments
    ///
    /// * `engine` - The object implementing the [Engine](crate::Engine) trait.
    ///
    /// * `args` - The arguments passed to the host function.
    ///
    /// # Error
    ///
    /// If fail to run the host function, then an error is returned.
    ///
    pub fn call<E: Engine>(
        &self,
        engine: &mut E,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        engine.run_func_ref(self, args)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        error::HostFuncError,
        params, Executor, ImportObjectBuilder, Statistics, Store, WasmVal,
    };
    use wasmedge_sys::WasmValue;

    #[test]
    fn test_func_signature() {
        // test signature with args and returns
        {
            let sig = FuncTypeBuilder::new()
                .with_args(vec![
                    ValType::I32,
                    ValType::I64,
                    ValType::F32,
                    ValType::F64,
                    ValType::V128,
                    ValType::FuncRef,
                    ValType::ExternRef,
                ])
                .with_returns(vec![ValType::FuncRef, ValType::ExternRef, ValType::V128])
                .build();

            // check the arguments
            let result = sig.args();
            assert!(result.is_some());
            let args = result.unwrap();
            assert_eq!(
                args,
                &[
                    ValType::I32,
                    ValType::I64,
                    ValType::F32,
                    ValType::F64,
                    ValType::V128,
                    ValType::FuncRef,
                    ValType::ExternRef,
                ]
            );

            // check the returns
            let result = sig.returns();
            assert!(result.is_some());
            let returns = result.unwrap();
            assert_eq!(
                returns,
                &[ValType::FuncRef, ValType::ExternRef, ValType::V128]
            );
        }

        // test signature without args and returns
        {
            let sig = FuncTypeBuilder::new().build();
            assert_eq!(sig.args(), None);
            assert_eq!(sig.returns(), None);
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_func_basic() {
        // create an ImportModule
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host func")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        // get the instance of the ImportObject module
        let result = store.module_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_some());
        let host_func = result.unwrap();

        // check the signature of the host function
        let result = host_func.ty();
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert!(func_ty.args().is_some());
        assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
        assert!(func_ty.returns().is_some());
        assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);

        // run the host function
        let result = host_func.call(&mut executor, params!(2, 3));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 5);
    }

    #[test]
    fn test_func_wrap() {
        // create a host function
        let result = Func::wrap::<(i32, i32), i32>(real_add);
        assert!(result.is_ok());
        let func = result.unwrap();

        // create an executor
        let mut executor = Executor::new(None, None).unwrap();

        // call the host function
        let result = func.call(&mut executor, params!(2, 3));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 5);
    }

    fn real_add(
        _: &CallingFrame,
        inputs: Vec<WasmValue>,
    ) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
        if inputs.len() != 2 {
            return Err(HostFuncError::User(1));
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(HostFuncError::User(2));
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(HostFuncError::User(3));
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
