use crate::{
    libmaccel::{self, fixedptc::Fpt},
    params::AllParamArgs,
    AccelParams, AccelParamsByMode, LinearCurveParams, NaturalCurveParams, SynchronousCurveParams,
};

use crate::AccelMode;

impl AllParamArgs {
    fn convert_to_accel_args(&self, mode: AccelMode) -> AccelParams {
        let params_by_mode = match mode {
            AccelMode::Linear => AccelParamsByMode::Linear(LinearCurveParams {
                accel: self.accel,
                offset_linear: self.offset_linear,
                output_cap: self.output_cap,
            }),
            AccelMode::Natural => AccelParamsByMode::Natural(NaturalCurveParams {
                decay_rate: self.decay_rate,
                offset_natural: self.offset_natural,
                limit: self.limit,
            }),
            AccelMode::Synchronous => AccelParamsByMode::Synchronous(SynchronousCurveParams {
                gamma: self.gamma,
                smooth: self.smooth,
                motivity: self.motivity,
                sync_speed: self.sync_speed,
            }),
        };

        AccelParams {
            sens_mult: self.sens_mult,
            yx_ratio: self.yx_ratio,
            input_dpi: self.input_dpi,
            by_mode: params_by_mode,
        }
    }
}

pub type SensXY = (f64, f64);

/// Ratio of Output speed to Input speed
pub fn sensitivity(s_in: f64, mode: AccelMode, params: &AllParamArgs) -> SensXY {
    let sens =
        unsafe { libmaccel::sensitivity_rs(s_in.into(), params.convert_to_accel_args(mode)) };
    let ratio_x: f64 = Fpt(sens.x).into();
    let ratio_y: f64 = Fpt(sens.y).into();

    (ratio_x, ratio_y)
}
