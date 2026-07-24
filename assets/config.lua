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
			continue_text = "CONTINUE",
			load_text = "LOAD",
			new_text = "NEW GAME",
			exit_text = "EXIT",
		},
		-- The in-game pause overlay.
		pause_menu = {
			title_text = "PAUSED",
			title_size = 48,
			resume_text = "RESUME",
			save_text = "SAVE",
			quit_text = "QUIT TO MENU",
			saved_notice = "saved",
			saved_notice_seconds = 2.0, -- how long the "saved" confirmation shows
			scrim_alpha = 180,          -- 0-255 dimming behind the overlay
		},
		-- The load / new-game slot chooser.
		slot_picker = {
			load_title = "LOAD GAME",
			new_title = "NEW GAME",
			title_size = 48,
			back_text = "BACK",
			empty_text = "- empty -",
			-- Slot rows are a wide list, distinct from the shared button layout.
			row_width = 700,
			row_height = 48,
			row_gap = 10,
		},
		-- The "save before quitting?" modal shown from the pause menu.
		confirm_quit = {
			title_text = "QUIT TO MENU",
			title_size = 28,
			message_text = "Save before quitting?",
			message_size = 22,
			confirm_text = "YES",
			cancel_text = "NO",
			box_width = 560,
			box_height = 240,
			scrim_alpha = 180, -- 0-255 dimming behind the modal
		},
	},
	debug = {
		show_fps = false,
	},
	save = {
		slots = 6,
	},
}
