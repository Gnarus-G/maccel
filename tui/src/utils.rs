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
    use maccel_core::{ContextRef, Parameter, fixedptc::Fpt};
    use mocks::MockStore;

    pub fn new_context() -> (ContextRef<MockStore>, Vec<Parameter>) {
        let params = [
            (maccel_core::Param::SensMult, 1.0),
            (maccel_core::Param::Accel, 1.0),
        ];

        let params = params.map(|(p, v)| (p, Fpt::from(v)));
        let context = ContextRef::new(
            maccel_core::TuiContext::init(
                MockStore {
                    list: params.to_vec(),
                },
                &params.map(|(p, _)| p),
            )
            .unwrap(),
        );

        (context, params.map(|(p, v)| Parameter::new(p, v)).to_vec())
    }

    mod mocks {
        use anyhow::Context;
        use maccel_core::{AccelMode, Param, fixedptc::Fpt, persist::ParamStore};

        #[derive(Debug)]
        pub struct MockStore {
            pub list: Vec<(Param, Fpt)>,
        }

        impl ParamStore for MockStore {
            fn set(&mut self, param: Param, value: f64) -> anyhow::Result<()> {
                if !self.list.iter().any(|(p, _)| p == &param) {
                    self.list.push((param, value.into()));
                }
                Ok(())
            }

            fn get(&self, param: Param) -> anyhow::Result<Fpt> {
                self.list
                    .iter()
                    .find(|(p, _)| p == &param)
                    .map(|(_, v)| v)
                    .copied()
                    .context("failed to get param")
            }

            fn set_current_accel_mode(
                &mut self,
                _mode: maccel_core::AccelMode,
            ) -> anyhow::Result<()> {
                unimplemented!()
            }
            fn get_current_accel_mode(&self) -> anyhow::Result<maccel_core::AccelMode> {
                Ok(AccelMode::Linear)
            }
        }
    }
}
