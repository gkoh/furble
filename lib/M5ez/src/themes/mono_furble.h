//  This is the dark theme. See how little you need to change for a different look?

#define TFT_FURBLE (0xFB60)  // color565(255,111,0)
{
  ezTheme theme;

  theme.name = "Mono Furble";  // Change this when making theme
  theme.background = TFT_BLACK;
  theme.foreground = TFT_WHITE;
  theme.header_bgcolor = TFT_BLACK;
  theme.header_fgcolor = TFT_FURBLE;
  theme.print_color = theme.foreground;
  theme.button_bgcolor_b = TFT_WHITE;
  theme.button_bgcolor_t = TFT_WHITE;
  theme.button_fgcolor = TFT_BLACK;
  theme.menu_item_color = theme.foreground;
  theme.menu_sel_bgcolor = theme.foreground;
  theme.menu_sel_fgcolor = theme.background;
  theme.msg_color = theme.foreground;
  theme.progressbar_color = theme.foreground;
  theme.battery_0_fgcolor = TFT_FURBLE;
  theme.battery_25_fgcolor = TFT_FURBLE;
  theme.battery_50_fgcolor = TFT_FURBLE;
  theme.battery_75_fgcolor = TFT_WHITE;
  theme.battery_100_fgcolor = TFT_WHITE;

  theme.add();
}
