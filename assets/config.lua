-- Vagrant Rim — game configuration.
-- Edited freely without recompiling; loaded at startup by src/config.
-- Visual theme (colors, widget text size) lives in the raygui style (.rgs),
-- not here. This file holds window, layout, content, and debug settings.
return {
	window = {
		width = 1280,
		height = 720,
		title = "Vagrant Rim",
		target_fps = 60,
	},
	ui = {
		-- raygui theme authored in rGuiStyler (https://raylibtech.itch.io/rguistyler).
		-- Path is resolved against the asset search paths; "" uses raygui's default.
		style_file = "styles/terminal.rgs",
		-- UI font. The .rgs can't supply a font in this toolchain, so we load
		-- our own. Swap to "fonts/VT323-Regular.ttf" for a retro CRT look.
		font_file = "fonts/ShareTechMono-Regular.ttf",
		font_size = 28,
		-- Shared button layout, reused by every menu. Colors/text size come
		-- from the .rgs style, not here.
		button = {
			width = 240,
			height = 48,
			gap = 14,
		},
		-- The loading / title screen. One of many menus.
		loading_menu = {
			title_text = "VAGRANT RIM",
			title_size = 72,
			tagline_text = "a space scavenger story",
			tagline_size = 20,
		},
	},
	debug = {
		show_fps = false,
	},
}
