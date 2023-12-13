import sublime
import sublime_plugin
import re
import os.path

# %appdata%/Sublime Text/Packages/User
# http://www.sublimetext.com/docs/api_reference.html

class FindResultsGotoCommand(sublime_plugin.TextCommand):
	def run(self, edit):
		view = self.view
		selection_range = view.sel()[0];
		file_path = self.get_file_path(selection_range)
		if file_path is not None:
			position = self.get_position(selection_range)
			if position is not None:
				view.window().open_file("%s:%s" % (file_path, position), sublime.ENCODED_POSITION)
			else:
				view.window().open_file(file_path)

	def get_file_path(self, target_range):
		view = self.view
		line_range = view.line(target_range)
		while line_range.begin() > 0:
			match = re.match(r"^(.+):$", view.substr(line_range))
			if match:
				if os.path.exists(match.group(1)):
					return match.group(1)
			line_range = view.line(line_range.begin() - 1)
		return None

	def get_position(self, target_range):
		view = self.view
		line_text = view.substr(view.line(target_range))
		match = re.match(r"^ *(\d+)[: ] ", line_text)
		if match:
			column = view.rowcol(target_range.begin())[1] + 1 - len(match.group(0))
			return "%s:%s" % (match.group(1), column)
		return None

class BuildOutputGotoCommand(sublime_plugin.TextCommand):
	def run(self, edit):
		view = self.view
		selection_range = view.sel()[0];
		file_target = self.get_file_target_clang(selection_range)
		if file_target is None:
			file_target = self.get_file_target_msvc(selection_range)
		if file_target is not None:
			view.window().open_file(file_target, sublime.ENCODED_POSITION)

	def get_file_target_clang(self, target_range):
		view = self.view
		line_range = view.line(target_range)
		while line_range.begin() > 0:
			match = re.match(r"^(.+):(\d+:\d+).*$", view.substr(line_range))
			if match:
				if os.path.exists(match.group(1)):
					return "%s:%s" % (os.path.abspath(match.group(1)), match.group(2))
			line_range = view.line(line_range.begin() - 1)
		return None

	def get_file_target_msvc(self, target_range):
		view = self.view
		line_range = view.line(target_range)
		while line_range.begin() > 0:
			match = re.match(r"^(.+)\((\d+),?(\d+)?\).*$", view.substr(line_range))
			if match:
				if os.path.exists(match.group(1)):
					return "%s:%s:%s" % (os.path.abspath(match.group(1)), match.group(2), match.group(3))
			line_range = view.line(line_range.begin() - 1)
		return None

class MoveViewCommand(sublime_plugin.WindowCommand):

	def run(self, position):
		view = self.window.active_view()
		group, index = self.window.get_view_index(view)
		if index < 0: return

		if isinstance(position, str) and position[0] in '-+':
			position = index + int(position)

		count = len(self.window.views_in_group(group))
		position = min(max(0, int(position)), count - 1)

		if position != index:
			self.window.set_view_index(view, group, position)
			self.window.focus_view(view)

	def is_enabled(self):
		(group, index) = self.window.get_view_index(self.window.active_view())
		return -1 not in (group, index) and len(self.window.views_in_group(group)) > 1

class SetFontSizeCommand(sublime_plugin.ApplicationCommand):
	def run(self, font_size = None):
		s = sublime.load_settings("Preferences.sublime-settings")
		if not font_size:
			s.erase("font_size")
		else:
			s.set('font_size', font_size)

		sublime.save_settings("Preferences.sublime-settings")
