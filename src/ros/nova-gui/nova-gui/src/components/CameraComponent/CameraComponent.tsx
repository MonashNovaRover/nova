import {
  Button,
  Card,
  CardFooter,
  Popover,
  PopoverContent,
  PopoverTrigger,
  Spinner,
} from "@nextui-org/react";
import React, { ReactNode, useCallback, useEffect, useRef, useState } from "react";
import { Camera as CameraIcon, Info } from "react-feather";
import { CameraInfoModal } from "./components/CameraInfoModal";
import { StreamingState, useCameraStream } from "./hooks/useCameraStream";
import { CameraSettingsForm } from "./components/CameraSettingsForm";
import CameraVideo, {CameraVideoProps} from "./components/CameraVideo";
import {
  defaultCamFilters,
  initialisedFilters
} from "../../views/shared/CamerasPage/CameraFilterConstants";
import { BooleanChip } from "./components/BooleanChip";
import humanizeString from "humanize-string";
import { ExternalLink } from "react-feather";
import toast from "react-hot-toast";
import CameraSessionStartStopButton from "./components/CameraSessionStartStopButton.tsx";

const ASPECT_RATIO = 4 / 3;

/// Subset of props needed to call the camera components from the serialMappedCameraComponent function
export interface BaseCameraComponentProps {
  cameraSerial: string;
  autostart?: boolean;
}

export interface CameraComponentProps extends BaseCameraComponentProps {
  // Children to pass to the settings form
  settingsFormChildren?: ReactNode

  cameraVideoComponent?: React.FC<CameraVideoProps>
}

export interface CameraFilters {
  flipCamera: boolean;
  invertCamera: boolean;
  rotation: number; // between -180 to 180
  contrast: number; // between 0 and 200
  brightness: number; // between 0 and 200
}

const getInitialFilters = (cameraSerial: string): CameraFilters => {
  return defaultCamFilters[cameraSerial] || initialisedFilters;
}

export const CameraComponent = (props: CameraComponentProps) => {
  const { cameraSerial, autostart: allCamerasStarted } = props;
  const cameraName = humanizeString(cameraSerial);
  const cardRef = useRef<HTMLDivElement>(null);
  const [isHovered, setIsHovered] = useState(false);
  const [isCameraInfoModalOpen, setCameraInfoModalOpen] = useState(false);
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const {
    streamingState,
    sendSessionStartMessage,
    isCameraOnline,
    closeSession,
  } = useCameraStream(cameraSerial, videoRef, allCamerasStarted);
  const [isSettingsOpen, setSettingsOpen] = useState(false);
  const [filters, setFilters] = useState(getInitialFilters(cameraSerial));

  const openCameraInTab = () =>
    window.open(
      `/cameras/${cameraSerial}`,
      "_blank",
      "rel=noopener noreferrer"
    );

  const takeScreenshot = useCallback(async () => {
    if (videoRef.current && window) {
      const video = videoRef.current;
      const canvas = document.createElement("canvas");
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      const context = canvas.getContext("2d");
      if (context) {
        context.drawImage(video, 0, 0);
        const blob = await new Promise<Blob | null>((resolve) => {
          canvas.toBlob((blob) => resolve(blob));
        });
        if (blob) {
          const url = URL.createObjectURL(blob);
          const link = document.createElement("a");
          link.href = url;
          link.download = `${cameraSerial}-${Date.now()}.png`;
          link.click();
        }
      }
    } else {
      toast("Unable to Take a Screenshot");
    }
  }, [videoRef, cameraSerial]);

  useEffect(() => {
    const handleMouseEnter = () => {
      setIsHovered(true);
    };

    const handleMouseLeave = () => {
      setIsHovered(false);
    };

    const cardElement = cardRef.current;
    if (cardElement) {
      cardElement.addEventListener("mouseenter", handleMouseEnter);
      cardElement.addEventListener("mouseleave", handleMouseLeave);
    }

    return () => {
      if (cardElement) {
        cardElement.removeEventListener("mouseenter", handleMouseEnter);
        cardElement.removeEventListener("mouseleave", handleMouseLeave);
      }
    };
  }, []);

  const cameraVideo = props.cameraVideoComponent?.({videoRef, filters}) ?? (
    <CameraVideo videoRef={videoRef} filters={filters} />
  )

  return (
    <Card className={` aspect-[${ASPECT_RATIO}] `} ref={cardRef}>
      <CameraInfoModal
        {...props}
        cameraName={cameraName}
        isModalOpen={isCameraInfoModalOpen}
        setCameraModalOpen={setCameraInfoModalOpen}
      />
      <div/>

      {cameraVideo}

      {/* Overlay */}
      <div className="absolute top-0 right-0 w-full h-full flex flex-col justify-center items-center">
        {streamingState === StreamingState.STOPPED && (
          <div className="flex flex-col gap-1 items-center">
            <div className="font-bold text-xl">{cameraName}</div>
            <BooleanChip
              boolean={isCameraOnline}
              trueText="Online"
              falseText="Offline"
              variant="dot"
              size="md"
            />
          </div>
        )}
        {streamingState === StreamingState.LOADING && <Spinner />}
      </div>

      <CardFooter className="absolute z-1 bottom-0 bg-gradient-to-t from-black/100 to-black/15">
        <div className="w-full flex flex-row justify-between px-1 items-center">
          <div className="text-lg font-semibold py-1">{cameraName}</div>
          {(isHovered || isSettingsOpen) && (
            <div className="flex flex-ow gap-2">
              <CameraSessionStartStopButton streamingState={streamingState}
                                            sendSessionStartMessage={sendSessionStartMessage}
                                            closeSession={closeSession}/>
              <Button isIconOnly size="sm" onPress={takeScreenshot}>
                <CameraIcon size="15px"/>
              </Button>
              <Button isIconOnly size="sm" onPress={openCameraInTab}>
                <ExternalLink size="15px" />
              </Button>
              <Popover
                placement="bottom"
                size="lg"
                isOpen={isSettingsOpen}
                onOpenChange={(open) => setSettingsOpen(open)}
              >
                <PopoverTrigger>
                  <Button isIconOnly size="sm">
                    <Info size="15px" />
                  </Button>
                </PopoverTrigger>
                <PopoverContent className="w-[360px] dark text-foreground">
                  <div className="px-1 py-2 w-full">
                    <CameraSettingsForm
                      cameraFilters={filters}
                      setCameraFilters={setFilters}
                    >
                      {props.settingsFormChildren}
                    </CameraSettingsForm>
                  </div>
                </PopoverContent>
              </Popover>
            </div>
          )}
        </div>
      </CardFooter>
    </Card>
  );
};
