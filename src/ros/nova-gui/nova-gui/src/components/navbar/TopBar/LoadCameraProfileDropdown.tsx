import {
  Dropdown,
  DropdownTrigger,
  DropdownMenu,
  DropdownItem,
  Button,
} from "@nextui-org/react";
import { Trash2 } from "react-feather";
import { useGenericStore } from "../../../hooks/useGenericStore";
import { CameraProfilesState } from "../../../redux/models/CameraProfilesState";
import { emitLoadProfileEvent } from "../../../utils/cameraProfileEvents";
import toast from "react-hot-toast";

interface LoadCameraProfileDropdownProps {
  onLoad?: (profileName: string) => void;
}

export const LoadCameraProfileDropdown: React.FC<LoadCameraProfileDropdownProps> = ({
  onLoad,
}) => {
  const [cameraProfiles, setCameraProfiles] = useGenericStore<CameraProfilesState>("cameraProfiles");
  const profiles = Object.values(cameraProfiles.profiles);

  const handleLoad = (profileName: string) => {
    const profile = cameraProfiles.profiles[profileName];
    if (profile) {
      const cameraCount = Object.keys(profile.cameras).length;
      console.log(`Loading profile ${profileName} with cameras:`, profile.cameras);
      toast.success(`Loading ${profileName} (${cameraCount} cameras)`);
      
      // Small delay to ensure all camera components are mounted and listening
      setTimeout(() => {
        // Emit event to notify all cameras to load the profile
        emitLoadProfileEvent(profileName);
      }, 100);
      
      if (onLoad) {
        onLoad(profileName);
      }
    }
  };

  const handleDelete = (profileName: string, e?: React.MouseEvent) => {
    e?.stopPropagation();
    const { [profileName]: _, ...remainingProfiles } = cameraProfiles.profiles;
    setCameraProfiles({
      profiles: remainingProfiles,
    });
    toast.success(`Deleted profile: ${profileName}`);
  };

  if (profiles.length === 0) {
    return (
      <Button
        radius="sm"
        size="sm"
        variant="shadow"
        isDisabled
      >
        No Profiles
      </Button>
    );
  }

  return (
    <Dropdown placement="bottom-end">
      <DropdownTrigger>
        <Button
          radius="sm"
          size="sm"
          variant="shadow"
        >
          Load Profile
        </Button>
      </DropdownTrigger>
      <DropdownMenu 
        aria-label="Camera Profiles"
        onAction={(key) => handleLoad(key as string)}
      >
        {profiles.map((profile) => (
          <DropdownItem
            key={profile.name}
            description={new Date(profile.timestamp).toLocaleString()}
            endContent={
              <Button
                isIconOnly
                size="sm"
                variant="light"
                color="danger"
                onPress={() => handleDelete(profile.name)}
              >
                <Trash2 size={16} />
              </Button>
            }
          >
            {profile.name}
          </DropdownItem>
        ))}
      </DropdownMenu>
    </Dropdown>
  );
};
