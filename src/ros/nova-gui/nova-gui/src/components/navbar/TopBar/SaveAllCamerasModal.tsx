import {
  Modal,
  ModalContent,
  ModalHeader,
  ModalBody,
  ModalFooter,
  Button,
  Input,
} from "@nextui-org/react";
import { useState, useEffect, useRef } from "react";
import { useGenericStore } from "../../../hooks/useGenericStore";
import { CameraProfilesState, CameraProfile, CameraSettings } from "../../../redux/models/CameraProfilesState";
import { CameraProfileEvents, emitSaveProfileEvent } from "../../../utils/cameraProfileEvents";

interface SaveAllCamerasModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSave?: (profileName: string) => void;
}

export const SaveAllCamerasModal: React.FC<SaveAllCamerasModalProps> = ({
  isOpen,
  onClose,
  onSave,
}) => {
  const [profileName, setProfileName] = useState("");
  const [errorMessage, setErrorMessage] = useState("");
  const [collectedCameras, setCollectedCameras] = useState<{ [serial: string]: CameraSettings }>({});
  const [isCollecting, setIsCollecting] = useState(false);
  const collectedCamerasRef = useRef<{ [serial: string]: CameraSettings }>({});
  
  const [cameraProfiles, setCameraProfiles] = useGenericStore<CameraProfilesState>("cameraProfiles");
  const existingProfiles = cameraProfiles.profiles;

  // Keep ref in sync with state
  useEffect(() => {
    collectedCamerasRef.current = collectedCameras;
  }, [collectedCameras]);

  // Listen for camera filters ready events
  useEffect(() => {
    const handleFiltersReady = (event: Event) => {
      const customEvent = event as CustomEvent<{ cameraSerial: string; filters: CameraSettings }>;
      const { cameraSerial, filters } = customEvent.detail;
      
      console.log(`Modal received filters from ${cameraSerial}:`, filters);
      
      setCollectedCameras(prev => ({
        ...prev,
        [cameraSerial]: filters,
      }));
    };

    window.addEventListener(CameraProfileEvents.FILTERS_READY, handleFiltersReady);
    
    return () => {
      window.removeEventListener(CameraProfileEvents.FILTERS_READY, handleFiltersReady);
    };
  }, []);

  const handleSave = () => {
    const trimmedName = profileName.trim();
    
    if (!trimmedName) {
      setErrorMessage("Profile name cannot be empty");
      return;
    }
    
    if (existingProfiles[trimmedName]) {
      setErrorMessage("A profile with this name already exists");
      return;
    }

    // Start collecting camera data
    setIsCollecting(true);
    setCollectedCameras({});
    
    // Emit event to request all cameras send their filters
    emitSaveProfileEvent(trimmedName);
    
    // Wait for cameras to respond, then save
    setTimeout(() => {
      setIsCollecting(false);
      
      // Use ref to get the most up-to-date collected cameras
      const camerasToSave = { ...collectedCamerasRef.current };
      
      const newProfile: CameraProfile = {
        name: trimmedName,
        timestamp: Date.now(),
        cameras: camerasToSave,
      };

      setCameraProfiles({
        profiles: {
          ...existingProfiles,
          [trimmedName]: newProfile,
        },
      });

      console.log("Saved profile:", trimmedName, "with cameras:", camerasToSave);

      if (onSave) {
        onSave(trimmedName);
      }

      // Reset and close
      setProfileName("");
      setErrorMessage("");
      setCollectedCameras({});
      collectedCamerasRef.current = {};
      onClose();
    }, 500); // Increased timeout to give cameras time to respond
  };

  const handleClose = () => {
    setProfileName("");
    setErrorMessage("");
    setCollectedCameras({});
    collectedCamerasRef.current = {};
    setIsCollecting(false);
    onClose();
  };

  return (
    <Modal
      isOpen={isOpen}
      onClose={handleClose}
      className="dark text-foreground"
      size="lg"
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">
          Save All Camera Settings
        </ModalHeader>

        <ModalBody>
          <Input
            label="Profile Name"
            placeholder="Enter a name for this profile"
            value={profileName}
            onValueChange={(value) => {
              setProfileName(value);
              setErrorMessage("");
            }}
            isInvalid={!!errorMessage}
            errorMessage={errorMessage}
            autoFocus
            isDisabled={isCollecting}
            variant="bordered"
          />
        </ModalBody>

        <ModalFooter className="flex justify-end gap-2">
          <Button
            variant="flat"
            color="default"
            onPress={handleClose}
            isDisabled={isCollecting}
          >
            Cancel
          </Button>

          <Button
            variant="flat"
            color="primary"
            onPress={handleSave}
            isDisabled={!profileName.trim() || isCollecting}
            isLoading={isCollecting}
          >
            Save Profile
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
};
