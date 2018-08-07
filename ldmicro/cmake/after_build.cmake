
message(STATUS "* After build *")

separate_arguments(_S)

file( GLOB_RECURSE _exe LIST_DIRECTORIES false ${pattern} )

#message(STATUS "${_exe}")

foreach( _e ${_exe})
	get_filename_component(_dir ${_e} DIRECTORY)
	message(STATUS "Output dir:${_dir}")
	
	message(STATUS "Copy to output:${_S}")
	file(COPY ${_S} DESTINATION ${_dir} )
	message(STATUS "Copy to output:${_D}")
	file(COPY ${_D} DESTINATION ${_dir} )
endforeach()