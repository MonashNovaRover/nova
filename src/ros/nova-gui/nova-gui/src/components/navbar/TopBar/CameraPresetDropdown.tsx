import {
  Dropdown,
  DropdownTrigger,
  DropdownMenu,
  DropdownItem,
  DropdownSection,
  Button,
} from "@nextui-org/react";
import { Trash2, Save, Upload } from "react-feather";
import { useGenericStore } from "../../../hooks/useGenericStore";
import { CameraProfilesState } from "../../../redux/models/CameraProfilesState";
import { emitLoadProfileEvent } from "../../../utils/cameraProfileEvents";
import toast from "react-hot-toast";
import { useState } from "react";

interface CameraPresetDropdownProps {
  onSavePress: () => void;
}

export const CameraPresetDropdown: React.FC<CameraPresetDropdownProps> = ({
  onSavePress,
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
    }
  };

  const handleDelete = (profileName: string) => {
    const { [profileName]: _, ...remainingProfiles } = cameraProfiles.profiles;
    setCameraProfiles({
      profiles: remainingProfiles,
    });
    toast.success(`Deleted profile: ${profileName}`);
  };

  return (
    <Dropdown placement="bottom-end">
      <DropdownTrigger>
        <Button
          size="md"
          color="primary"
          variant="ghost"
          className="w-36"
        >
          Preset
        </Button>
      </DropdownTrigger>
      <DropdownMenu 
        aria-label="Camera Preset Actions"
        className="max-h-[400px] overflow-y-auto"
      >
        <DropdownSection title="Actions" showDivider>
          <DropdownItem
            key="save"
            startContent={<Save size={16} />}
            onPress={onSavePress}
          >
            Save Preset
          </DropdownItem>
        </DropdownSection>
        <DropdownSection title="Load Preset">
          {profiles.length === 0 ? (
            <DropdownItem key="no-profiles" isReadOnly>
              No saved presets
            </DropdownItem>
          ) : (
            profiles.map((profile) => (
              <DropdownItem
                key={profile.name}
                description={new Date(profile.timestamp).toLocaleString()}
                startContent={<Upload size={16} />}
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
                onPress={() => handleLoad(profile.name)}
              >
                {profile.name}
              </DropdownItem>
            ))
          )}
        </DropdownSection>
      </DropdownMenu>
    </Dropdown>
  );
};
