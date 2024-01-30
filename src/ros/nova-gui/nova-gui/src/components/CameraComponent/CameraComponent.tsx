import {
  Button,
  Card,
  CardFooter,
  Popover,
  PopoverContent,
  PopoverTrigger,
  Spinner,
} from "@nextui-org/react";
import { useEffect, useRef, useState } from "react";
import { Camera as CameraIcon, Info, Play, Square } from "react-feather";
import { CameraInfoModal } from "./components/CameraInfoModal";
import { StreamingState, useCameraStream } from "./hooks/useCameraStream";
import { CameraSettingsForm } from "./components/CameraSettingsForm";
import CameraVideo from "./components/CameraVideo";

const ASPECT_RATIO = 4 / 3;

export interface CameraComponentProps {
  cameraName: string;
  cameraSerial: string;
}

export const CameraComponent = (props: CameraComponentProps) => {
  const { cameraName, cameraSerial } = props;
  const cardRef = useRef<HTMLDivElement>(null);
  const [isHovered, setIsHovered] = useState(false);
  const [isCameraInfoModalOpen, setCameraInfoModalOpen] = useState(false);
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const { streamingState, sendSessionStartMessage } = useCameraStream(
    cameraSerial,
    videoRef
  );
  const [isSettingsOpen, setSettingsOpen] = useState(false);

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

  return (
    <Card className={`m-4 h-[41vh] aspect-[${ASPECT_RATIO}] `} ref={cardRef}>
      <CameraInfoModal
        {...props}
        isModalOpen={isCameraInfoModalOpen}
        setCameraModalOpen={setCameraInfoModalOpen}
      />
      <CameraVideo videoRef={videoRef} />
      <div className="absolute top-0 right-0 w-full h-full flex flex-col justify-center items-center">
        {streamingState === StreamingState.STOPPED && (
          <Button
            size="sm"
            color="primary"
            className="w-min mx-auto"
            onClick={() => sendSessionStartMessage()}
          >
            <Play size="15px" fill="white" />
            Start
          </Button>
        )}
        {streamingState === StreamingState.LOADING && <Spinner />}
      </div>

      <CardFooter className="absolute z-1 bottom-0 bg-gradient-to-t from-black/100 to-black/15">
        <div className="w-full flex flex-row justify-between px-1 items-center">
          <div className="text-lg font-semibold py-1">{cameraName}</div>
          {(isHovered || isSettingsOpen) && (
            <div className="flex flex-ow gap-2">
              {streamingState === StreamingState.STOPPED ? (
                <Button
                  size="sm"
                  color="primary"
                  className="w-min mx-auto"
                  onClick={() => sendSessionStartMessage()}
                >
                  <Play size="15px" fill="white" />
                  Start
                </Button>
              ) : (
                <Button size="sm" color="danger" className="w-min mx-auto">
                  <Square size="15px" fill="white" /> Stop
                </Button>
              )}

              <Button isIconOnly size="sm">
                <CameraIcon size="15px" />
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
                    <CameraSettingsForm />
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
