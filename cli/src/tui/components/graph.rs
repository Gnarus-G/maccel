use crate::{
    inputspeed::read_input_speed,
    libmaccel::{sensitivity, SensitivityParams},
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

#[derive(Debug)]
pub struct Graph {
    last_mouse_move: LastMouseMove,
    output_cap: f64,
    sens_mult: f64,
    yx_ratio: f64,
    data: Vec<(f64, f64)>,
    data_alt: Vec<(f64, f64)>,
    title: &'static str,
    data_name: String,
    data_alt_name: String,
    context: ContextRef,
}

impl Graph {
    pub fn new(context: ContextRef) -> Self {
        let mut s = Self {
            context,
            last_mouse_move: Default::default(),
            output_cap: Param::OutputCap.get().unwrap_or_default().into(),
            sens_mult: Param::SensMult.get().unwrap_or_default().into(),
            yx_ratio: Param::YxRatio.get().unwrap_or_default().into(),
            data: vec![],
            data_alt: vec![],
            title: "graph (Sensitivity = Speed_out / Speed_in)",
            data_name: "Sens 🠠 🠢 ".to_string(),
            data_alt_name: "Sens 🠡 🠣".to_string(),
        };
        s.update_data();
        s
    }

    fn update_data(&mut self) {
        self.data.clear();
        self.data_alt.clear();

        for x in (0..900).map(|x| (x as f64) * 0.1375 /* step size */) {
            let sens_x = sensitivity(x, SensitivityParams::new());
            self.data.push((x, sens_x));
            let sens_y = sens_x * self.yx_ratio;
            self.data_alt.push((x, sens_y));
        }
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
            self.last_mouse_move = LastMouseMove {
                in_speed,
                out_sens_x: out_sens,
                out_sens_y: out_sens * self.yx_ratio,
            };

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

            self.yx_ratio = self
                .context
                .get()
                .parameters
                .iter()
                .find(|p| p.param == Param::YxRatio)
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

        let highlight_point_x = &[self.last_mouse_move.as_point_sens_x()];
        let highlight_point_y = &[self.last_mouse_move.as_point_sens_y()];
        let chart = Chart::new(vec![
            Dataset::default()
                .name(self.data_name.clone())
                .marker(symbols::Marker::Braille)
                .graph_type(GraphType::Line)
                .style(Style::default().green())
                .data(&self.data),
            Dataset::default()
                .name(self.data_alt_name.clone())
                .marker(symbols::Marker::Braille)
                .graph_type(GraphType::Line)
                .style(Style::default().yellow())
                .data(&self.data_alt),
            // current instance of user input speed and output sensitivity
            Dataset::default()
                .marker(symbols::Marker::Braille)
                .style(Style::default().red())
                .data(highlight_point_x),
            Dataset::default()
                .marker(symbols::Marker::Braille)
                .style(Style::default().blue())
                .data(highlight_point_y),
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
    (
        input_speed,
        sensitivity(input_speed, SensitivityParams::new()),
    )
}
