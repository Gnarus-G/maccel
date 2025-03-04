use crate::{
    libmaccel::{
        c_libmaccel::{self, LinearCurveArgs, NaturalCurveArgs},
        fixedptc::Fixedpt,
    },
    params::AllParamArgs,
};

use super::context::AccelMode;

impl AllParamArgs {
    fn convert_to_accel_args(&self, mode: AccelMode) -> c_libmaccel::AccelArgs {
        let choice_args = match mode {
            AccelMode::Linear => c_libmaccel::AccelArgsChoice::Linear(LinearCurveArgs {
                accel: self.accel,
                offset: self.offset,
                output_cap: self.output_cap,
            }),
            AccelMode::Natural => c_libmaccel::AccelArgsChoice::Natural(NaturalCurveArgs {
                decay_rate: self.decay_rate,
                offset: self.offset,
                limit: self.limit,
            }),
        };

        c_libmaccel::AccelArgs {
            param_sens_mult: self.sens_mult,
            param_yx_ratio: self.yx_ratio,
            args: choice_args,
        }
    }
}

pub type SensXY = (f64, f64);

/// Ratio of Output speed to Input speed
pub fn sensitivity(s_in: f64, mode: AccelMode, params: &AllParamArgs) -> SensXY {
    let s_in: Fixedpt = s_in.into();
    let sens = unsafe { c_libmaccel::sensitivity_rs(s_in.0, params.convert_to_accel_args(mode)) };
    let ratio_x: f64 = Fixedpt(sens.x).into();
    let ratio_y: f64 = Fixedpt(sens.y).into();

    (ratio_x, ratio_y)
}
