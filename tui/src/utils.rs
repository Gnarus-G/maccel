#[derive(Debug)]
pub struct CyclingIdx {
    idx: usize,
    n: usize,
}

impl CyclingIdx {
    pub fn new(n: usize) -> Self {
        Self::new_starting_at(n, 0)
    }

    pub fn new_starting_at(n: usize, idx: usize) -> Self {
        Self { idx, n }
    }

    pub fn current(&self) -> usize {
        debug_assert!(self.idx < self.n);
        self.idx
    }

    pub fn forward(&mut self) {
        self.idx += 1;
        if self.idx >= self.n {
            self.idx = 0
        }
    }

    pub fn back(&mut self) {
        if self.idx == 0 {
            self.idx = self.n
        }
        self.idx -= 1
    }
}

#[cfg(test)]
pub(crate) mod test_utils {
    use maccel_core::{fixedptc::Fixedpt, ContextRef, Parameter};
    use mocks::MockStore;

    pub fn new_context() -> (ContextRef<MockStore>, Vec<Parameter>) {
        let params = [
            (maccel_core::Param::SensMult, 1.0),
            (maccel_core::Param::Accel, 1.0),
        ];

        let params = params.map(|(p, v)| (p, Fixedpt::from(v)));
        let context = ContextRef::new(maccel_core::TuiContext::new(
            MockStore {
                list: params.to_vec(),
            },
            &params.map(|(p, _)| p),
        ));

        (context, params.map(|(p, v)| Parameter::new(p, v)).to_vec())
    }

    mod mocks {
        use anyhow::Context;
        use maccel_core::{fixedptc::Fixedpt, persist::ParamStore, AccelMode, Param};

        #[derive(Debug)]
        pub struct MockStore {
            pub list: Vec<(Param, Fixedpt)>,
        }

        impl ParamStore for MockStore {
            fn set(&mut self, param: Param, value: f64) -> anyhow::Result<()> {
                if !self.list.iter().any(|(p, _)| p == &param) {
                    self.list.push((param, value.into()));
                }
                Ok(())
            }

            fn get(&self, param: &Param) -> anyhow::Result<Fixedpt> {
                self.list
                    .iter()
                    .find(|(p, _)| p == param)
                    .map(|(_, v)| v)
                    .copied()
                    .context("failed to get param")
            }

            fn set_current_accel_mode(_mode: maccel_core::AccelMode) {
                unimplemented!()
            }
            fn get_current_accel_mode() -> maccel_core::AccelMode {
                AccelMode::Linear
            }
        }
    }
}
