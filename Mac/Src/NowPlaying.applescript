
on change_case(this_text, this_case)
	if this_case is 0 then
		set the comparison_string to "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		set the source_string to "abcdefghijklmnopqrstuvwxyz"
	else
		set the comparison_string to "abcdefghijklmnopqrstuvwxyz"
		set the source_string to "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	end if
	set the new_text to ""
	repeat with this_char in this_text
		set x to the offset of this_char in the comparison_string
		if x is not 0 then
			set the new_text to (the new_text & character x of the source_string) as string
		else
			set the new_text to (the new_text & this_char) as string
		end if
	end repeat
	return the new_text
end change_case

on replace_chars(this_text, search_string, replacement_string)
	set AppleScript's text item delimiters to the search_string
	set the item_list to every text item of this_text
	set AppleScript's text item delimiters to the replacement_string
	set this_text to the item_list as string
	set AppleScript's text item delimiters to ""
	return this_text
end replace_chars

on export_artwork(artworkWidth, artworkDirectory)
	
	set myResult to "" as string
	set artworkFolder to POSIX file artworkDirectory as string
	
	tell application "iTunes"
		
		if player state is not stopped then
			
			set itunesTrack to current track
			set theName to (artist of itunesTrack & album of itunesTrack) as string
			set theNameEscaped to theName as string
			set theNameEscaped to my replace_chars(theNameEscaped, "\"", "\\\"")
			
			set c to my change_case(do shell script "md5 -q -s \"" & my replace_chars(theName, "\"", "\\\"") & "\"", 0)
			
			if (class of itunesTrack is file track and artworks of itunesTrack is not {} and kind of itunesTrack is not "QuickTime movie file" and theName is not "") then
				
				set artworkFormat to (format of artwork 1 of itunesTrack) as string
				if artworkFormat contains "JPEG" then
					set extension to ".jpg"
				else if artworkFormat contains "PNG" then
					set extension to ".png"
				else
					return myResult
				end if
				
				-- set filepath to target_folder
				set finalartworkFile to (artworkFolder & "now_playing-" & c & extension) as string
				
				-- write temp file
				set file_reference to (open for access finalartworkFile write permission 1)
				write (get raw data of artwork 1 of itunesTrack) to file_reference starting at 0
				close access file_reference
				
				if artworkWidth is not 0 then
					tell application "Image Events"
						launch
						-- open the image file
						set this_image to open finalartworkFile
						-- perform action
						scale this_image to size artworkWidth
						-- save the changes
						save this_image with icon
						-- purge the open image data
						close this_image
					end tell
				end if
				
				set myResult to POSIX path of finalartworkFile
				
			end if
			
		end if
		
	end tell
	
	return myResult
	
end export_artwork