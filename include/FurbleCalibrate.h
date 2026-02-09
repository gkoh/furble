#ifndef FURBLE_CALIBRATE_H
#define FURBLE_CALIBRATE_H

#include <lvgl.h>

#include "icons.h"

#include <array>

namespace Furble {
class CalibrationUI {
 public:
  CalibrationUI(const int32_t width, const int32_t height);

  /**
   * Perform touch screen calibration.
   *
   * Does not return, can only restart.
   */
  void calibrate(void);

 private:
  typedef struct {
    lv_obj_t *icon;
    const lv_image_dsc_t *image;
    const int32_t icon_x;
    const int32_t icon_y;
    lv_obj_t *spinbox_x;
    lv_obj_t *spinbox_y;
    lv_subject_t subject_x;
    lv_subject_t subject_y;
    const int32_t spinbox_x_x;
    const int32_t spinbox_x_y;
  } corner_calibration_t;

  static constexpr int32_t ICON_ARROW_SIZE = 20;

  const int32_t m_Width;
  const int32_t m_Height;

  std::array<corner_calibration_t, 4> m_Corners = {
      {{
           // top left
           .icon = NULL,
           .image = &icon_north_west_20,
           .icon_x = 0,
           .icon_y = 0,
           .spinbox_x = NULL,
           .spinbox_y = NULL,
           .subject_x = {},
           .subject_y = {},
           .spinbox_x_x = 30,
           .spinbox_x_y = 20,
       }, {
           // bottom left
           .icon = NULL,
           .image = &icon_south_west_20,
           .icon_x = 0,
           .icon_y = m_Height - ICON_ARROW_SIZE,
           .spinbox_x = NULL,
           .spinbox_y = NULL,
           .subject_x = {},
           .subject_y = {},
           .spinbox_x_x = 30,
           .spinbox_x_y = 145,
       }, {
           // top right
           .icon = NULL,
           .image = &icon_north_east_20,
           .icon_x = m_Width - ICON_ARROW_SIZE,
           .icon_y = 0,
           .spinbox_x = NULL,
           .spinbox_y = NULL,
           .subject_x = {},
           .subject_y = {},
           .spinbox_x_x = 230,
           .spinbox_x_y = 20,
       }, {
           // bottom right
           .icon = NULL,
           .image = &icon_south_east_20,
           .icon_x = m_Width - ICON_ARROW_SIZE,
           .icon_y = m_Height - ICON_ARROW_SIZE,
           .spinbox_x = NULL,
           .spinbox_y = NULL,
           .subject_x = {},
           .subject_y = {},
           .spinbox_x_x = 230,
           .spinbox_x_y = 145,
       }}
  };

  lv_obj_t *m_Screen = nullptr;
  lv_obj_t *m_Save = nullptr;
  lv_obj_t *m_Clear = nullptr;
  lv_obj_t *m_Restart = nullptr;
};

}  // namespace Furble

#endif
