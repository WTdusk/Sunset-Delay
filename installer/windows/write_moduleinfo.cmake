# Writes Contents/Resources/moduleinfo.json for a Windows VST3 bundle.
# Class IDs follow VST3Interface::jucePluginId() with the Windows byte order
# (JUCE_WINDOWS). Manufacturer Wtea, plugin Gdly. VERSION defaults to 1.2.0.
# A MinGW cross build cannot run JUCE's Windows manifest helper, so packaging
# calls this script. A native Windows build already has the helper output.

if(NOT DEFINED BUNDLE_DIR OR BUNDLE_DIR STREQUAL "")
    message(FATAL_ERROR "BUNDLE_DIR is required")
endif()

if(NOT DEFINED VERSION OR VERSION STREQUAL "")
    set(VERSION "1.2.0")
endif()

get_filename_component(BUNDLE_DIR "${BUNDLE_DIR}" ABSOLUTE)
set(info_dir "${BUNDLE_DIR}/Contents/Resources")
file(MAKE_DIRECTORY "${info_dir}")

# component  01EFCDAB8291EBFA + Wtea + Gdly
# controller 01EFCDAB3412CDAB + Wtea + Gdly
set(json "{
  \"Name\": \"Sunset Delay\",
  \"Version\": \"${VERSION}\",
  \"Factory Info\": {
    \"Vendor\": \"White Tea\",
    \"URL\": \"\",
    \"E-Mail\": \"\",
    \"Flags\": {
      \"Unicode\": true,
      \"Classes Discardable\": false,
      \"Component Non Discardable\": false
    }
  },
  \"Classes\": [
    {
      \"CID\": \"01EFCDAB8291EBFA5774656147646C79\",
      \"Category\": \"Audio Module Class\",
      \"Name\": \"Sunset Delay\",
      \"Vendor\": \"White Tea\",
      \"Version\": \"${VERSION}\",
      \"SDKVersion\": \"VST 3.8.0\",
      \"Sub Categories\": [\"Fx\", \"Delay\"],
      \"Class Flags\": 0,
      \"Cardinality\": 2147483647,
      \"Snapshots\": []
    },
    {
      \"CID\": \"01EFCDAB3412CDAB5774656147646C79\",
      \"Category\": \"Component Controller Class\",
      \"Name\": \"Sunset Delay\",
      \"Vendor\": \"White Tea\",
      \"Version\": \"${VERSION}\",
      \"SDKVersion\": \"VST 3.8.0\",
      \"Sub Categories\": [\"Fx\", \"Delay\"],
      \"Class Flags\": 0,
      \"Cardinality\": 2147483647,
      \"Snapshots\": []
    }
  ]
}
")
file(WRITE "${info_dir}/moduleinfo.json" "${json}")
message(STATUS "Wrote ${info_dir}/moduleinfo.json")
