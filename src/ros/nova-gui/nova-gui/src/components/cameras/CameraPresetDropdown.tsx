import {
  Dropdown,
  DropdownTrigger,
  DropdownMenu,
  DropdownItem,
  DropdownSection,
  Button,
} from "@nextui-org/react";
import { Trash2, Save, Upload, User, Check } from "react-feather";
import { useGenericStore } from "../../hooks/useGenericStore";
import { CameraProfilesState } from "../../redux/models/CameraProfilesState";
import { emitLoadProfileEvent } from "../../utils/cameraProfileEvents";
import toast from "react-hot-toast";
import { useEffect } from "react";

interface CameraPresetDropdownProps {
  onSavePress: () => void;
}

export const CameraPresetDropdown: React.FC<CameraPresetDropdownProps> = ({
  onSavePress,
}) => {
  const [cameraProfiles, setCameraProfiles] = useGenericStore<CameraProfilesState>("cameraProfiles");
  const profiles = Object.values(cameraProfiles.profiles).sort((a, b) => b.timestamp - a.timestamp);

  // Auto-load last loaded profile on mount
  useEffect(() => {
    if (cameraProfiles.lastLoadedProfile && cameraProfiles.profiles[cameraProfiles.lastLoadedProfile]) {
      // Small delay to ensure all camera components are mounted and listening
      setTimeout(() => {
        emitLoadProfileEvent(cameraProfiles.lastLoadedProfile!);
      }, 100);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []); // Empty dependency array - only run on mount

  const handleLoad = (profileName: string) => {
    const profile = cameraProfiles.profiles[profileName];
    if (profile) {
      const cameraCount = Object.keys(profile.cameras).length;
      toast.success(`Loading ${profileName} (${cameraCount} cameras)`);
      
      // Update last loaded profile
      setCameraProfiles({
        ...cameraProfiles,
        lastLoadedProfile: profileName,
      });
      
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
      lastLoadedProfile: cameraProfiles.lastLoadedProfile === profileName ? null : cameraProfiles.lastLoadedProfile,
    });
    toast.success(`Deleted profile: ${profileName}`);
  };

  return (
    <Dropdown placement="bottom-end">
      <DropdownTrigger>
        <Button
          isIconOnly
          size="md"
          color="primary"
          variant="ghost"
        >
          <User size={20} />
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
                startContent={
                  cameraProfiles.lastLoadedProfile === profile.name ? (
                    <Check size={16} className="text-success" />
                  ) : (
                    <Upload size={16} />
                  )
                }
                className={cameraProfiles.lastLoadedProfile === profile.name ? "bg-primary/10" : ""}
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
