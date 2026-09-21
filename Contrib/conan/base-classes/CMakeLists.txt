cmake_minimum_required(VERSION 4.0)

project(base-classes)


set(SRC_LIST 
		amextra.cpp
		amfilter.cpp
		amvideo.cpp
		arithutil.cpp
		combase.cpp
		cprop.cpp
		ctlutil.cpp
		ddmm.cpp
		dllentry.cpp
		dllsetup.cpp
		mtype.cpp
		outputq.cpp
		perflog.cpp
		pstream.cpp
		pullpin.cpp
		refclock.cpp
		renbase.cpp
		schedule.cpp
		seekpt.cpp
		source.cpp
		strmctl.cpp
		sysclock.cpp
		transfrm.cpp
		transip.cpp
		videoctl.cpp
		vtrans.cpp
		winctrl.cpp
		winutil.cpp
		wxdebug.cpp
		wxlist.cpp
		wxutil.cpp
	)

	set(HEADER_FILES  
			amextra.h
			amfilter.h
			cache.h
			combase.h
			cprop.h
			ctlutil.h
			ddmm.h
			dllsetup.h
			dxmperf.h
			fourcc.h
			measure.h
			msgthrd.h
			mtype.h
			outputq.h
			perflog.h
			perfstruct.h
			pstream.h
			pullpin.h
			refclock.h
			reftime.h
			renbase.h
			schedule.h
			seekpt.h
			source.h
			streams.h
			strmctl.h
			sysclock.h
			transfrm.h
			transip.h
			videoctl.h
			vtrans.h
			winctrl.h
			winutil.h
			wxdebug.h
			wxlist.h
			wxutil.h
)

include_directories(${CMAKE_SOURCE_DIR}/)

add_definitions(-DUNICODE -D_UNICODE)

add_library(base-classes STATIC ${SRC_LIST})
