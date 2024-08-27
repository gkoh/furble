//  This is the dark theme. See how little you need to change for a different look?

{
  ezTheme theme;

  theme.name = "Dark";  // Change this when making theme
  theme.background = TFT_BLACK;
  theme.foreground = TFT_WHITE;
  theme.header_bgcolor = TFT_DARKGREY;
  theme.header_fgcolor = TFT_WHITE;
  theme.print_color = theme.foreground;
  theme.menu_item_color = theme.foreground;
  theme.menu_sel_bgcolor = theme.foreground;
  theme.menu_sel_fgcolor = theme.background;
  theme.msg_color = theme.foreground;
  theme.progressbar_color = theme.foreground;

  theme.add();
}
