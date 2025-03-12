use std::fmt::Debug;

use maccel_core::{ContextRef, SensXY, get_param_value_from_ctx, persist::ParamStore, sensitivity};

use crate::{action, component::TuiComponent};

use crossterm::event::KeyEvent;
use ratatui::{prelude::*, widgets::*};
use tracing::debug;

#[derive(Debug, Default)]
struct LastMouseMove {
    in_speed: f64,
    out_sens_x: f64,
    out_sens_y: f64,
}

impl LastMouseMove {
    fn as_point_sens_x(&self) -> (f64, f64) {
        (self.in_speed, self.out_sens_x)
    }

    fn as_point_sens_y(&self) -> (f64, f64) {
        (self.in_speed, self.out_sens_y)
    }
}

pub type GetNewYAxisBounds<PS> = dyn FnMut(ContextRef<PS>) -> [f64; 2];

pub struct SensitivityGraph<PS: ParamStore> {
    last_mouse_move: LastMouseMove,
    pub y_bounds: [f64; 2],
    data: Vec<(f64, f64)>,
    data_alt: Vec<(f64, f64)>,
    title: &'static str,
    data_name: String,
    data_alt_name: String,
    context: ContextRef<PS>,
    on_y_axis_update_fn: Box<GetNewYAxisBounds<PS>>,
}

impl<PS: ParamStore> SensitivityGraph<PS> {
    pub fn new(context: ContextRef<PS>) -> Self {
        Self {
            context: context.clone(),
            last_mouse_move: Default::default(),
            y_bounds: [0.0, 0.0],
            data: vec![],
            data_alt: vec![],
            title: "Sensitivity Graph (Ratio = Speed_out / Speed_in)",
            data_name: "🠠🠢 Sens".to_string(),
            data_alt_name: "🠡🠣 Sens".to_string(),
            on_y_axis_update_fn: Box::new(|ctx| {
                [
                    0.0,
                    f64::from(get_param_value_from_ctx!(ctx, SensMult)) * 2.0,
                ]
            }),
        }
    }

    pub fn on_y_axix_bounds_update(
        mut self,
        updater: impl FnMut(ContextRef<PS>) -> [f64; 2] + 'static,
    ) -> Self {
        self.on_y_axis_update_fn = Box::new(updater);
        self
    }

    fn update_data(&mut self) {
        self.data.clear();
        self.data_alt.clear();

        let params = self.context.get().params_snapshot();
        for x in (0..128).map(|x| (x as f64) * 1.0 /* step size */) {
            let (sens_x, sens_y) =
                maccel_core::sensitivity(x, self.context.get().current_mode, &params);
            self.data.push((x, sens_x));
            if sens_x != sens_y {
                self.data_alt.push((x, sens_y));
            }
        }
    }

    fn read_input_speed_and_resolved_sens(&self) -> (f64, SensXY) {
        let input_speed = maccel_core::inputspeed::read_input_speed();
        let params = self.context.get().params_snapshot();
        debug!("last mouse move read at {} counts/ms", input_speed);
        (
            input_speed,
            sensitivity(input_speed, self.context.get().current_mode, &params),
        )
    }

    fn update_last_move(&mut self) {
        let (in_speed, out_sens) = self.read_input_speed_and_resolved_sens();
        self.last_mouse_move = LastMouseMove {
            in_speed,
            out_sens_x: out_sens.0,
            out_sens_y: out_sens.1,
        };
    }

    fn formatted_current_point(&self, x: f64, y: f64) -> String {
        format!("• ({:0<6.3}, {:0<6.3})", x, y)
    }
}

impl<PS: ParamStore> TuiComponent for SensitivityGraph<PS> {
    fn handle_key_event(&mut self, _event: &KeyEvent, _actions: &mut action::Actions) {}

    fn handle_mouse_event(
        &mut self,
        _event: &crossterm::event::MouseEvent,
        _actions: &mut action::Actions,
    ) {
    }

    fn update(&mut self, action: &action::Action) {
        if let action::Action::Tick = action {
            debug!("updating graph on tick");
            self.update_last_move();
            self.update_data();
            self.y_bounds = (self.on_y_axis_update_fn)(self.context.clone());
        }
    }

    fn draw(&self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        let (bounds, labels) = bounds_and_labels([0.0, 128.0], 16);
        let x_axis = Axis::default()
            .title("Speed_in".magenta())
            .style(Style::default().white())
            .bounds(bounds)
            .labels(labels);

        let (bounds, labels) = bounds_and_labels(self.y_bounds, 10);
        let y_axis = Axis::default()
            .title("Ratio".magenta())
            .style(Style::default().white())
            .bounds(bounds)
            .labels(labels);

        let highlight_point_x = &[self.last_mouse_move.as_point_sens_x()];
        let highlight_point_y = &[self.last_mouse_move.as_point_sens_y()];
        let mut chart_plots = vec![
            Dataset::default()
                .name(if self.data_alt.is_empty() {
                    "🠠🠢 🠡🠣Sens".to_owned()
                } else {
                    self.data_name.clone()
                })
                .marker(symbols::Marker::Braille)
                .graph_type(GraphType::Line)
                .style(Style::default().green())
                .data(&self.data),
            // current instance of user input speed and output sensitivity
            Dataset::default()
                .name(self.formatted_current_point(
                    self.last_mouse_move.in_speed,
                    self.last_mouse_move.out_sens_x,
                ))
                .marker(symbols::Marker::Braille)
                .style(Style::default().red())
                .data(highlight_point_x),
        ];

        if !self.data_alt.is_empty() {
            chart_plots.push(
                Dataset::default()
                    .name(self.data_alt_name.clone())
                    .marker(symbols::Marker::Braille)
                    .graph_type(GraphType::Line)
                    .style(Style::default().yellow())
                    .data(&self.data_alt),
            );

            chart_plots.push(
                Dataset::default()
                    .name(self.formatted_current_point(
                        self.last_mouse_move.in_speed,
                        self.last_mouse_move.out_sens_y,
                    ))
                    .marker(symbols::Marker::Braille)
                    .style(Style::default().blue().bold())
                    .data(highlight_point_y),
            );
        }

        let chart = Chart::new(chart_plots).x_axis(x_axis).y_axis(y_axis);

        frame.render_widget(
            chart.block(
                Block::default()
                    .borders(Borders::NONE)
                    .title(self.title)
                    .bold(),
            ),
            area,
        );
    }
}

fn bounds_and_labels(bounds: [f64; 2], div: usize) -> ([f64; 2], Vec<Span<'static>>) {
    let [o, f] = bounds;
    let d = f - o;
    let step = d / (div as f64);

    let labels = (0..=div)
        .map(|i| o + i as f64 * step)
        .map(|label| format!("{:.2}", label).into())
        .collect();

    (bounds, labels)
}
