use maccel_core::{get_param_value_from_ctx, persist::ParamStore, ContextRef};

use crate::{component::TuiComponent, graph::Graph};

#[derive(Debug)]
pub struct NaturalCurveGraph<PS: ParamStore> {
    context: ContextRef<PS>,
    limit: f64,
    sens_mult: f64,
    graph: Graph<PS>,
}

impl<PS: ParamStore> NaturalCurveGraph<PS> {
    pub fn new(context: ContextRef<PS>) -> Self {
        let mut s = Self {
            limit: get_param_value_from_ctx!(context, Limit).into(),
            sens_mult: get_param_value_from_ctx!(context, SensMult).into(),
            graph: Graph::new(
                context.clone(),
                "Sensitivity Graph (Ratio = Speed_out / Speed_in)",
                "🠠🠢 Sens".to_string(),
                "🠡🠣 Sens".to_string(),
            ),
            context,
        };

        s.set_graph_y_bounds();

        s
    }

    fn set_graph_y_bounds(&mut self) {
        self.graph.y_bounds = [0.0, self.sens_mult * self.limit.max(1.0) * 2.0];
    }
}

impl<PS: ParamStore> TuiComponent for NaturalCurveGraph<PS> {
    fn handle_key_event(
        &mut self,
        event: &crossterm::event::KeyEvent,
        actions: &mut super::action::Actions,
    ) {
        self.graph.handle_key_event(event, actions);
    }

    fn handle_mouse_event(
        &mut self,
        event: &crossterm::event::MouseEvent,
        actions: &mut super::action::Actions,
    ) {
        self.graph.handle_mouse_event(event, actions);
    }

    fn update(&mut self, action: &super::action::Action) {
        self.sens_mult = get_param_value_from_ctx!(&self.context, SensMult).into();
        self.limit = get_param_value_from_ctx!(&self.context, Limit).into();
        self.set_graph_y_bounds();
        self.graph.update(action);
    }

    fn draw(&self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        self.graph.draw(frame, area);
    }
}
