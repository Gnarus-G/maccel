use ratatui::{prelude::*, widgets::Widget};

pub struct CustomScrollbar {
    total_items: usize,
    visible_items: usize,
    scroll_position: usize,
}

impl CustomScrollbar {
    pub fn new(total: usize, visible: usize, position: usize) -> Self {
        Self {
            total_items: total,
            visible_items: visible,
            scroll_position: position,
        }
    }

    pub fn with_position(self, position: usize) -> Self {
        Self {
            scroll_position: position,
            ..self
        }
    }
}

impl Widget for CustomScrollbar {
    fn render(self, area: Rect, buf: &mut Buffer) {
        if area.height < 3 || self.total_items == 0 {
            return;
        }

        let track_height = area.height.saturating_sub(2);
        let track_top = area.y + 1;
        let track_bottom = track_top + track_height.saturating_sub(1);
        let x = area.x;

        let max_scroll = self.total_items.saturating_sub(self.visible_items);
        let scroll_ratio = if max_scroll > 0 {
            (self.scroll_position as f64) / (max_scroll as f64)
        } else {
            0.0
        };

        let thumb_ratio = (self.visible_items as f64) / (self.total_items as f64);
        let thumb_height = ((track_height as f64) * thumb_ratio).max(1.0).ceil() as u16;
        let thumb_height = thumb_height.min(track_height);

        let thumb_start = if self.scroll_position >= max_scroll {
            track_bottom.saturating_sub(thumb_height - 1)
        } else {
            let available_space = track_height.saturating_sub(thumb_height);
            let thumb_offset = (scroll_ratio * (available_space as f64)).round() as u16;
            track_top + thumb_offset
        };

        buf.set_string(x, track_top.saturating_sub(1), "▲", Style::default());
        buf.set_string(x, track_bottom + 1, "▼", Style::default());

        for row in track_top..=track_bottom {
            let is_thumb = row >= thumb_start && row < thumb_start + thumb_height;
            let symbol = if is_thumb { "█" } else { "│" };
            let style = if is_thumb {
                Style::default().fg(Color::White)
            } else {
                Style::default().fg(Color::Gray)
            };
            buf.set_string(x, row, symbol, style);
        }
    }
}
