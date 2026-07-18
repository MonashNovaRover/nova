import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";
import {ProfileOption} from "../../../views/shared/CamerasPage/CameraProfileConstants.ts";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {useState} from "react";

interface CameraProfileSelectorProps {
  serials: string[]
  options: ProfileOption[]
  currentProfile?: string
}

export const CameraProfileSelector = (
  {serials, options, currentProfile}
  : CameraProfileSelectorProps) => {

  const bifrost = useBifrost({ service: RosService.PRESET_CAMS });

  const [altSelectedProfile, setAltSelectedProfile] = useState(0)

  const selectedIndex = currentProfile ?
    options.map(op => op.name).indexOf(currentProfile) :
    altSelectedProfile

  const onIndexChange = (index: number)=> {
    bifrost.callService({
      serials: serials,
      profile: options[index].name
    })

    if (!currentProfile)
      setAltSelectedProfile(index)
  }

  return (
    <SegmentedPicker
      selectedIndex={selectedIndex}
      onIndexChange={onIndexChange}
      children={
        options.map(op => op.displayName)
      }
      color="secondary"
      className="pb-0"
      fullWidth
      variant="bordered"
    />
  )
}