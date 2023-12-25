import { Button, Card, CardFooter, Image } from "@nextui-org/react";
import { useEffect, useRef, useState } from "react";
import { Info } from "react-feather";
import { CameraInfoModal } from "./CameraInfoModal";

export interface CameraComponentProps {
  cameraName: string;
  src: string;
}

export const CameraComponent = (props: CameraComponentProps) => {
  const { cameraName, src } = props;
  const cardRef = useRef<HTMLDivElement>(null);
  const [isHovered, setIsHovered] = useState(false);
  const [isCameraInfoModalOpen, setCameraInfoModalOpen] = useState(false);

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
    <>
      <Card isFooterBlurred isHoverable className="m-4" ref={cardRef}>
        <CameraInfoModal
          {...props}
          isModalOpen={isCameraInfoModalOpen}
          setCameraModalOpen={setCameraInfoModalOpen}
        />
        <Image
          removeWrapper
          alt={cameraName}
          className="z-0 w-full h-full object-cover"
          src={src}
        />
        {isHovered && (
          <CardFooter className="absolute bottom-0">
            <div className="w-full flex flex-row justify-between px-2 items-center">
              <div className="font-semibold text-lg">{cameraName}</div>
              <Button isIconOnly onClick={() => setCameraInfoModalOpen(true)}>
                <Info />
              </Button>
            </div>
          </CardFooter>
        )}
      </Card>
    </>
  );
};
