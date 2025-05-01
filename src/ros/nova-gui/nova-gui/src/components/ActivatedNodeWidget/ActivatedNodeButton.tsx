import { Button, ButtonProps } from "@nextui-org/react"
import {ReactNode} from "react"
import Overlay from "../shared/Overlay/Overlay.tsx";
import {Lock} from "react-feather";

// Properties for the ActivatedNodeButton component.
export interface ActivatedNodeButtonProps extends ButtonProps {
  icon: ReactNode,
  text: string,
  isSelected: boolean,
  isLocked: boolean,
}

// A button used for displaying an active node for the ActivatedNodeWidget
export const ActivatedNodeButton: React.FC<ActivatedNodeButtonProps> = (props: ActivatedNodeButtonProps) => {
    return (
      <Overlay
        overlay={props.isLocked ? <div className="flex flex-col justify-center">
          <Lock/>
        </div>: undefined}
      >
        <Overlay
          overlay={props.isLocked ? <div className="grow backdrop-blur-[2px]"/> : undefined}
        >
          <Button
            color = {props.isSelected ? "primary" : "default"}
            variant = {props.isSelected ? "shadow" : "ghost"}
            isDisabled={true}
            className="grow opacity-100 w-full"
          >
            {props.icon}
            {props.text}
          </Button>
        </Overlay>
      </Overlay>
    );
  }

