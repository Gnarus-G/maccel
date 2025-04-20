use std::{
    cell::{Ref, RefCell, RefMut},
    ops::Deref,
    rc::Rc,
};

use anyhow::Context;

use crate::{
    AccelMode,
    libmaccel::fixedptc::Fpt,
    params::{AllParamArgs, Param},
    persist::ParamStore,
};

#[derive(Debug, Clone, Copy)]
pub struct Parameter {
    pub tag: Param,
    pub value: Fpt,
}

impl Parameter {
    pub fn new(param: Param, value: Fpt) -> Self {
        Self { tag: param, value }
    }
}

#[derive(Debug)]
pub struct TuiContext<PS: ParamStore> {
    pub current_mode: AccelMode,
    parameters: Vec<Parameter>,
    parameter_store: PS,
}

impl<PS: ParamStore> TuiContext<PS> {
    pub fn new(parameter_store: PS, parameters: &[Param]) -> Self {
        Self {
            current_mode: parameter_store
                .get_current_accel_mode()
                .expect("Failed to get accel mode from store while initializing TuiContext"),
            parameters: parameters
                .iter()
                .map(|&p| {
                    let value = parameter_store
                        .get(&p)
                        .expect("Failed to get a param from store while initializing TuiContext");
                    Parameter::new(p, value)
                })
                .collect(),
            parameter_store,
        }
    }

    pub fn parameter(&self, param: Param) -> Option<&Parameter> {
        self.parameters.iter().find(|p| p.tag == param)
    }

    pub fn update_param_value(&mut self, param_id: Param, value: f64) -> anyhow::Result<()> {
        let param = self
            .parameters
            .iter_mut()
            .find(|p| p.tag == param_id)
            .context("Unknown parameter, cannot update")?;

        self.parameter_store.set(param.tag, value)?;
        param.value = value.into();
        Ok(())
    }

    pub fn reset_current_parameters(&mut self) {
        for p in self.parameters.iter_mut() {
            p.value = self
                .parameter_store
                .get(&p.tag)
                .expect("failed to read and initialize a parameter's value");
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
            input_dpi: get!(InputDpi),
            accel: get!(Accel),
            offset_linear: get!(OffsetLinear),
            offset_natural: get!(OffsetNatural),
            output_cap: get!(OutputCap),
            decay_rate: get!(DecayRate),
            limit: get!(Limit),
            gamma: get!(Gamma),
            smooth: get!(Smooth),
            motivity: get!(Motivity),
            sync_speed: get!(SyncSpeed),
        }
    }
}

#[macro_export]
macro_rules! get_param_value_from_ctx {
    ($ctx:expr, $param_tag:tt) => {{
        use maccel_core::Param;
        let x = $ctx
            .get()
            .parameter(Param::$param_tag)
            .expect(concat!("failed to get param ", stringify!($param_tag)))
            .value;
        x
    }};
}

#[derive(Debug)]
pub struct ContextRef<PS: ParamStore> {
    inner: Rc<RefCell<TuiContext<PS>>>,
}

impl<PS: ParamStore> Clone for ContextRef<PS> {
    fn clone(&self) -> Self {
        Self {
            inner: Rc::clone(&self.inner),
        }
    }
}

impl<PS: ParamStore> ContextRef<PS> {
    pub fn new(value: TuiContext<PS>) -> Self {
        Self {
            inner: Rc::new(RefCell::new(value)),
        }
    }

    pub fn get(&self) -> Ref<'_, TuiContext<PS>> {
        self.inner.borrow()
    }

    pub fn get_mut(&mut self) -> RefMut<TuiContext<PS>> {
        self.inner.deref().borrow_mut()
    }
}
