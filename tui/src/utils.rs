use maccel_core::{get_paramater, AccelMode};

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

pub fn get_current_accel_mode() -> AccelMode {
    get_paramater(AccelMode::PARAM_NAME)
        .map(|mode_tag| match mode_tag.as_str() {
            "0" => AccelMode::Linear,
            "1" => AccelMode::Natural,
            id => unimplemented!("no mode id'd with {:?} exists", id),
        })
        .expect("Failed to read a kernel parameter to get the acceleration mode desired")
}
