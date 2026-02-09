#include <M5Unified.h>

#include "icons.h"

#include "FurbleCalibrate.h"
#include "FurbleSettings.h"

namespace Furble {
CalibrationUI::CalibrationUI(int32_t width, int32_t height) : m_Width(width), m_Height(height) {}

void CalibrationUI::calibrate(void) {
  m_Screen = lv_obj_create(NULL);

  Settings::calibration_t c = Settings::load<Settings::calibration_t>(Settings::TOUCH_CALIBRATION);
  lv_subject_init_int(&m_Corners[0].subject_x, c.top_left.x);
  lv_subject_init_int(&m_Corners[0].subject_y, c.top_left.y);
  lv_subject_init_int(&m_Corners[1].subject_x, c.bottom_left.x);
  lv_subject_init_int(&m_Corners[1].subject_y, c.bottom_left.y);
  lv_subject_init_int(&m_Corners[2].subject_x, c.top_right.x);
  lv_subject_init_int(&m_Corners[2].subject_y, c.top_right.y);
  lv_subject_init_int(&m_Corners[3].subject_x, c.bottom_right.x);
  lv_subject_init_int(&m_Corners[3].subject_y, c.bottom_right.y);

  lv_screen_load(m_Screen);

  for (auto &corner : m_Corners) {
    corner.icon = lv_image_create(m_Screen);
    lv_obj_set_size(corner.icon, ICON_ARROW_SIZE, ICON_ARROW_SIZE);
    lv_image_set_src(corner.icon, corner.image);
    lv_obj_set_pos(corner.icon, corner.icon_x, corner.icon_y);
    lv_obj_add_flag(corner.icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        corner.icon,
        [](lv_event_t *e) {
          auto *c = static_cast<corner_calibration_t *>(lv_event_get_user_data(e));
          auto touch = M5.Touch.getDetail(0);

          ESP_LOGI("calibrate", "%d, %d", touch.x, touch.y);
          lv_subject_set_int(&c->subject_x, touch.x);
          lv_subject_set_int(&c->subject_y, touch.y);
        },
        LV_EVENT_CLICKED, &corner);

    corner.spinbox_x = lv_spinbox_create(m_Screen);
    lv_spinbox_set_range(corner.spinbox_x, 0, m_Width);
    lv_spinbox_set_digit_format(corner.spinbox_x, 3, 0);
    lv_obj_set_style_text_font(corner.spinbox_x, &lv_font_montserrat_16, 0);
    lv_obj_set_size(corner.spinbox_x, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(corner.spinbox_x, corner.spinbox_x_x, corner.spinbox_x_y);
    lv_spinbox_bind_value(corner.spinbox_x, &corner.subject_x);

    lv_obj_t *x_decr = lv_button_create(m_Screen);
    lv_obj_set_style_bg_image_src(x_decr, LV_SYMBOL_MINUS, 0);
    lv_obj_set_style_text_font(x_decr, &lv_font_montserrat_12, 0);
    lv_obj_set_size(x_decr, ICON_ARROW_SIZE, ICON_ARROW_SIZE);
    lv_obj_align_to(x_decr, corner.spinbox_x, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_add_subject_increment_event(x_decr, &corner.subject_x, LV_EVENT_CLICKED, -1);

    lv_obj_t *x_incr = lv_button_create(m_Screen);
    lv_obj_set_style_bg_image_src(x_incr, LV_SYMBOL_PLUS, 0);
    lv_obj_set_style_text_font(x_incr, &lv_font_montserrat_12, 0);
    lv_obj_set_size(x_incr, ICON_ARROW_SIZE, ICON_ARROW_SIZE);
    lv_obj_align_to(x_incr, corner.spinbox_x, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_add_subject_increment_event(x_incr, &corner.subject_x, LV_EVENT_CLICKED, 1);

    corner.spinbox_y = lv_spinbox_create(m_Screen);
    lv_spinbox_set_range(corner.spinbox_y, 0, m_Height);
    lv_spinbox_set_digit_format(corner.spinbox_y, 3, 0);
    lv_obj_set_style_text_font(corner.spinbox_y, &lv_font_montserrat_16, 0);
    lv_obj_set_size(corner.spinbox_y, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(corner.spinbox_y, corner.spinbox_x_x, corner.spinbox_x_y + 40);
    lv_spinbox_bind_value(corner.spinbox_y, &corner.subject_y);

    lv_obj_t *y_decr = lv_button_create(m_Screen);
    lv_obj_set_style_bg_image_src(y_decr, LV_SYMBOL_MINUS, 0);
    lv_obj_set_style_text_font(y_decr, &lv_font_montserrat_12, 0);
    lv_obj_set_size(y_decr, ICON_ARROW_SIZE, ICON_ARROW_SIZE);
    lv_obj_align_to(y_decr, corner.spinbox_y, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_add_subject_increment_event(y_decr, &corner.subject_y, LV_EVENT_CLICKED, -1);

    lv_obj_t *y_incr = lv_button_create(m_Screen);
    lv_obj_set_style_bg_image_src(y_incr, LV_SYMBOL_PLUS, 0);
    lv_obj_set_style_text_font(y_incr, &lv_font_montserrat_12, 0);
    lv_obj_set_size(y_incr, ICON_ARROW_SIZE, ICON_ARROW_SIZE);
    lv_obj_align_to(y_incr, corner.spinbox_y, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_add_subject_increment_event(y_incr, &corner.subject_y, LV_EVENT_CLICKED, 1);
  }

  m_Save = lv_button_create(m_Screen);
  lv_obj_t *label_save = lv_label_create(m_Save);
  lv_label_set_text(label_save, "Save");
  lv_obj_set_size(m_Save, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  m_Clear = lv_button_create(m_Screen);
  lv_obj_t *label_clear = lv_label_create(m_Clear);
  lv_label_set_text(label_clear, "Clear");
  lv_obj_set_size(m_Clear, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  m_Restart = lv_button_create(m_Screen);
  lv_obj_t *label_restart = lv_label_create(m_Restart);
  lv_label_set_text(label_restart, "Restart");
  lv_obj_set_size(m_Restart, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

  lv_obj_center(m_Save);
  lv_obj_align_to(m_Clear, m_Save, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_align_to(m_Restart, m_Save, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  lv_obj_add_event_cb(
      m_Clear,
      [](lv_event_t *e) {
        auto *calibrationUI = static_cast<CalibrationUI *>(lv_event_get_user_data(e));
        for (auto &corner : calibrationUI->m_Corners) {
          lv_subject_set_int(&corner.subject_x, 0);
          lv_subject_set_int(&corner.subject_y, 0);
        }
        Settings::calibration_t calibration = {};
        Settings::save<Settings::calibration_t>(Settings::TOUCH_CALIBRATION, calibration);
      },
      LV_EVENT_CLICKED, this);

  lv_obj_add_event_cb(
      m_Save,
      [](lv_event_t *e) {
        auto *calibrationUI = static_cast<CalibrationUI *>(lv_event_get_user_data(e));
        Settings::calibration_t calibration = {
            .top_left =
                {static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[0].subject_x)),
                           static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[0].subject_y))},
            .bottom_left =
                {static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[1].subject_x)),
                           static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[1].subject_y))},
            .top_right =
                {static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[2].subject_x)),
                           static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[2].subject_y))},
            .bottom_right =
                {static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[3].subject_x)),
                           static_cast<uint16_t>(lv_subject_get_int(&calibrationUI->m_Corners[3].subject_y))},
            .calibrated = true,
        };
        Settings::save<Settings::calibration_t>(Settings::TOUCH_CALIBRATION, calibration);
      },
      LV_EVENT_CLICKED, this);

  lv_obj_add_event_cb(m_Restart, [](lv_event_t *e) { esp_restart(); }, LV_EVENT_CLICKED, NULL);
}
};  // namespace Furble
