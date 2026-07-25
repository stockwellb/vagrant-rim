-- Vagrant Rim — game configuration.
-- Edited freely without recompiling; loaded at startup by src/config.
-- Visual theme (colors, widget text size) lives in the raygui style (.rgs),
-- not here. This file holds window, layout, content, and debug settings.
return {
	window = {
		width = 1280,  -- windowed size (and the size before going fullscreen)
		height = 720,
		title = "Vagrant Rim",
		target_fps = 60,
		fullscreen = true, -- start in borderless fullscreen at the monitor's resolution
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
			settings_text = "SETTINGS",
			exit_text = "EXIT",
		},
		-- The in-game pause overlay.
		pause_menu = {
			title_text = "PAUSED",
			title_size = 48,
			resume_text = "RESUME",
			save_text = "SAVE",
			settings_text = "SETTINGS",
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
		-- The settings screen (volume sliders + mute), reachable from the main
		-- and pause menus.
		settings_menu = {
			title_text = "SETTINGS",
			title_size = 48,
			music_label = "MUSIC",
			sfx_label = "SFX",
			mute_label = "MUTE",
			mute_on_text = "ON",
			mute_off_text = "OFF",
			back_text = "BACK",
		},
	},
	audio = {
		-- Background music: a looping track resolved against the asset search
		-- paths. Drop a file at assets/audio/music.ogg (or point elsewhere); if
		-- it's missing the game simply runs without music. raylib supports
		-- .ogg / .mp3 / .wav / .flac / .xm / .mod.
		music_file = "audio/music.ogg",
		-- Menu focus-move sound (plays when the highlight moves between buttons
		-- via keyboard, gamepad, or mouse). If missing, a synthesized thud is
		-- used instead, so navigation always has feedback.
		nav_sfx_file = "audio/focus.ogg",
		-- Menu activate/confirm sound (plays when a button is chosen). If
		-- missing, a synthesized confirm tone is used instead.
		select_sfx_file = "audio/select.ogg",
		music_volume = 0.5, -- 0.0 .. 1.0
		sfx_volume = 0.7,   -- 0.0 .. 1.0
		muted = false,
	},
	debug = {
		show_fps = false,
	},
	save = {
		slots = 6,
	},
}
