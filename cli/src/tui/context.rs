use std::{
    cell::{Ref, RefCell, RefMut},
    ops::Deref,
    rc::Rc,
};

use anyhow::Context;

use crate::{
    libmaccel::fixedptc::Fixedpt,
    params::{AllParamArgs, Param},
};

#[derive(Debug, Clone, Copy)]
pub struct Parameter {
    pub tag: Param,
    pub value: Fixedpt,
}

impl Parameter {
    pub fn new(param: Param, value: Fixedpt) -> Self {
        Self { tag: param, value }
    }

    #[cfg(test)]
    pub fn new_with_float_value(param: Param, value: f64) -> Self {
        Self::new(param, value.into())
    }

    fn reset(&mut self) {
        self.value = self
            .tag
            .get()
            .expect("failed to read and initialize a parameter's value")
            .into();
    }
}

impl From<Param> for Parameter {
    fn from(param: Param) -> Self {
        let value = param
            .get()
            .expect("failed to read and initialize a parameter's value");

        Self::new(param, value)
    }
}

#[derive(Debug, Default, PartialEq, Clone, Copy)]
#[repr(C)]
pub enum AccelMode {
    #[default]
    Linear,
    Natural,
}

impl AccelMode {
    pub const PARAM_NAME: &'static str = "MODE";

    pub fn ordinal(&self) -> i64 {
        (*self as i8).into()
    }
}

#[derive(Debug)]
pub struct TuiContext {
    pub current_mode: AccelMode,
    pub parameters: Vec<Parameter>,
}

impl TuiContext {
    pub fn parameter(&self, param: Param) -> Option<&Parameter> {
        self.parameters.iter().find(|p| p.tag == param)
    }

    pub fn update_param_value(&mut self, param_id: Param, value: f64) -> anyhow::Result<()> {
        let param = self
            .parameters
            .iter_mut()
            .find(|p| p.tag == param_id)
            .context("Unknown parameter, cannot update")?;

        param.value = value.into();
        param.tag.set(value)
    }

    pub fn reset_current_parameters(&mut self) {
        for p in self.parameters.iter_mut() {
            p.reset();
        }
    }

    pub fn params_snapshot(&self) -> AllParamArgs {
        macro_rules! get {
            ($param_tag:tt) => {{
                use $crate::params::Param;
                let x = self
                    .parameter(Param::$param_tag)
                    .expect(concat!("failed to get param ", stringify!($param_tag)))
                    .value;
                x
            }};
        }

        AllParamArgs {
            sens_mult: get!(SensMult),
            yx_ratio: get!(YxRatio),
            accel: get!(Accel),
            offset: get!(Offset),
            output_cap: get!(OutputCap),
            decay_rate: get!(DecayRate),
            limit: get!(Limit),
        }
    }
}

#[macro_export]
macro_rules! get_param_value_from_ctx {
    ($ctx:expr, $param_tag:tt) => {{
        use $crate::params::Param;
        let x = $ctx
            .get()
            .parameter(Param::$param_tag)
            .expect(concat!("failed to get param ", stringify!($param_tag)))
            .value;
        x
    }};
}

#[derive(Debug)]
pub struct ContextRef {
    inner: Rc<RefCell<TuiContext>>,
}

impl Clone for ContextRef {
    fn clone(&self) -> Self {
        Self {
            inner: Rc::clone(&self.inner),
        }
    }
}

impl ContextRef {
    pub fn new(value: TuiContext) -> Self {
        Self {
            inner: Rc::new(RefCell::new(value)),
        }
    }

    pub fn get(&self) -> Ref<'_, TuiContext> {
        self.inner.borrow()
    }

    pub fn get_mut(&mut self) -> RefMut<TuiContext> {
        self.inner.deref().borrow_mut()
    }
}
