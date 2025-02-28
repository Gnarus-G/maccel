use std::{
    cell::{Ref, RefCell, RefMut},
    ops::Deref,
    rc::Rc,
};

use crate::params::Param;

#[derive(Debug)]
pub struct Parameter {
    pub param: Param,
    pub value: f64,
}

impl Parameter {
    pub fn new(param: Param, value: f64) -> Self {
        Self { param, value }
    }
}

impl From<Param> for Parameter {
    fn from(param: Param) -> Self {
        let value = param
            .get()
            .expect("failed to read and initialize a parameter's value")
            .into();

        Self::new(param, value)
    }
}

#[derive(Debug)]
pub struct Context {
    pub parameters: Vec<Parameter>,
}

#[derive(Debug)]
pub struct ContextRef {
    inner: Rc<RefCell<Context>>,
}

impl Clone for ContextRef {
    fn clone(&self) -> Self {
        Self {
            inner: Rc::clone(&self.inner),
        }
    }
}

impl ContextRef {
    pub fn new(value: Context) -> Self {
        Self {
            inner: Rc::new(RefCell::new(value)),
        }
    }

    pub fn get(&self) -> Ref<'_, Context> {
        self.inner.borrow()
    }

    pub fn get_mut(&mut self) -> RefMut<Context> {
        self.inner.deref().borrow_mut()
    }
}
