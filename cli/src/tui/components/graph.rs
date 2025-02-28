use crate::{
    inputspeed::read_input_speed,
    libmaccel::{sensitivity, Params},
    params::Param,
    tui::{
        action::{self, Action},
        component::TuiComponent,
        context::ContextRef,
        event,
    },
};

use ratatui::{prelude::*, widgets::*};
use tracing::debug;

#[derive(Debug, Default)]
struct LastMouseMove {
    in_speed: f64,
    out_sens: f64,
}

impl LastMouseMove {
    fn as_point(&self) -> (f64, f64) {
        (self.in_speed, self.out_sens)
    }
}

#[derive(Debug)]
pub struct Graph {
    last_mouse_move: LastMouseMove,
    output_cap: f64,
    sens_mult: f64,
    data: Vec<(f64, f64)>,
    title: &'static str,
    legend: String,
    context: ContextRef,
}

impl Graph {
    pub fn new(context: ContextRef) -> Self {
        let mut s = Self {
            context,
            last_mouse_move: Default::default(),
            output_cap: Param::OutputCap.get().unwrap_or_default().into(),
            sens_mult: Param::SensMult.get().unwrap_or_default().into(),
            data: vec![],
            title: "graph (Sensitivity = Speed_out / Speed_in)",
            legend: format!(
                "f(x) = (1 + {}⋅x) ⋅ {}",
                Param::Accel.display_name(),
                Param::SensMult.display_name()
            ),
        };
        s.update_data();
        s
    }

    fn update_data(&mut self) {
        self.data = (0..1000)
            .map(|x| (x as f64) * 0.1375)
            .map(|x| (x, sensitivity(x, Params::new())))
            .collect();
    }

    /// To make the y axis bounds and labels scale with our sensitivity multiplier
    fn auto_scaled_y_bounds(&self) -> [f64; 2] {
        [0.0, self.sens_mult * self.output_cap * 2.0]
    }
}

impl TuiComponent for Graph {
    fn handle_key_event(&mut self, _event: &event::KeyEvent, _actions: &mut action::Actions) {}

    fn handle_mouse_event(
        &mut self,
        _event: &crossterm::event::MouseEvent,
        _actions: &mut action::Actions,
    ) {
    }

    fn update(&mut self, action: &action::Action) {
        if let Action::Tick = action {
            let (in_speed, out_sens) = read_input_speed_and_resolved_sens();
            self.last_mouse_move = LastMouseMove { in_speed, out_sens };

            self.sens_mult = self
                .context
                .get()
                .parameters
                .iter()
                .find(|p| p.param == Param::SensMult)
                .map(|p| p.value)
                .unwrap_or_default();

            self.output_cap = self
                .context
                .get()
                .parameters
                .iter()
                .find(|p| p.param == Param::OutputCap)
                .map(|p| p.value)
                .unwrap_or_default();

            self.update_data();
        }
    }

    fn draw(&mut self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        let (bounds, labels) = bounds_and_labels([0.0, 128.0], 16);
        let x_axis = Axis::default()
            .title("Speed_in".magenta())
            .style(Style::default().white())
            .bounds(bounds)
            .labels(labels);

        let (bounds, labels) = bounds_and_labels(self.auto_scaled_y_bounds(), 5);
        let y_axis = Axis::default()
            .title("Sensitivity".magenta())
            .style(Style::default().white())
            .bounds(bounds)
            .labels(labels);

        let highlight_point = &[self.last_mouse_move.as_point()];
        let chart = Chart::new(vec![
            Dataset::default()
                .name(self.legend.clone())
                .marker(symbols::Marker::Braille)
                .graph_type(GraphType::Line)
                .style(Style::default().green())
                .data(&self.data),
            Dataset::default() // current instance of acceleration
                .marker(symbols::Marker::Braille)
                .style(Style::default().red())
                .data(highlight_point),
        ])
        .x_axis(x_axis)
        .y_axis(y_axis);

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

pub fn read_input_speed_and_resolved_sens() -> (f64, f64) {
    let input_speed = read_input_speed();
    debug!("last mouse move read at {} counts/ms", input_speed);
    (input_speed, sensitivity(input_speed, Params::new()))
}
