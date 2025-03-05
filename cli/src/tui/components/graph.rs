use crate::{
    inputspeed::read_input_speed,
    tui::{
        action::{self, Action},
        component::TuiComponent,
        context::ContextRef,
        event,
        sens_fns::{sensitivity, SensXY},
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
    pub y_bounds: [f64; 2],
    data: Vec<(f64, f64)>,
    data_alt: Vec<(f64, f64)>,
    title: &'static str,
    data_name: String,
    data_alt_name: String,
    context: ContextRef,
}

impl Graph {
    pub fn new(
        context: ContextRef,
        title: &'static str,
        data_name: String,
        data_alt_name: String,
    ) -> Self {
        Self {
            context: context.clone(),
            last_mouse_move: Default::default(),
            y_bounds: [0.0, 0.0],
            data: vec![],
            data_alt: vec![],
            title,
            data_name,
            data_alt_name,
        }
    }

    fn update_data(&mut self) {
        self.data.clear();
        self.data_alt.clear();

        let params = self.context.get().params_snapshot();
        for x in (0..128).map(|x| (x as f64) * 1.0 /* step size */) {
            let (sens_x, sens_y) = sensitivity(x, self.context.get().current_mode, &params);
            self.data.push((x, sens_x));
            if sens_x != sens_y {
                self.data_alt.push((x, sens_y));
            }
        }
    }

    fn read_input_speed_and_resolved_sens(&self) -> (f64, SensXY) {
        let input_speed = read_input_speed();
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
            debug!("updating graph on tick");
            self.update_last_move();
            self.update_data();
        }
    }

    fn draw(&self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        let (bounds, labels) = bounds_and_labels([0.0, 128.0], 16);
        let x_axis = Axis::default()
            .title("Speed_in".magenta())
            .style(Style::default().white())
            .bounds(bounds)
            .labels(labels);

        let (bounds, labels) = bounds_and_labels(self.y_bounds, 5);
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
                .name(format!(
                    "• ({:.3}, {:.3})",
                    self.last_mouse_move.in_speed, self.last_mouse_move.out_sens_x
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
                    .name(format!(
                        "• ({:.3}, {:.3})",
                        self.last_mouse_move.in_speed, self.last_mouse_move.out_sens_y
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
