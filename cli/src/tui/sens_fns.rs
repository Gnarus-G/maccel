use crate::{
    libmaccel::{c_libmaccel, fixedptc::Fixedpt},
    params::AllParamArgs,
};

pub type SensXY = (f64, f64);

/// Ratio of Output speed to Input speed
pub fn sensitivity(s_in: f64, params: &AllParamArgs) -> SensXY {
    let s_in: Fixedpt = s_in.into();
    let sens = unsafe {
        c_libmaccel::sensitivity_rs(
            s_in.0,
            params.sens_mult.0,
            params.yx_ratio.0,
            params.accel.0,
            params.offset.0,
            params.output_cap.0,
        )
    };
    let ratio_x: f64 = Fixedpt(sens.x).into();
    let ratio_y: f64 = Fixedpt(sens.y).into();

    (ratio_x, ratio_y)
}
