import sublime
import sublime_plugin
import re
import os.path

# http://www.sublimetext.com/docs/3/api_reference.html

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
