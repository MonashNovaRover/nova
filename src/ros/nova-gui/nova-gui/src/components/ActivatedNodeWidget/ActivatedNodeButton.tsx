import { Button, ButtonProps } from "@nextui-org/react"
import { ReactNode } from "react"

// Properties for the ActivatedNodeButton component.
export interface ActivatedNodeButtonProps extends ButtonProps {
  icon: ReactNode,
  text: string,
  isSelected: boolean,
}

// A button used for displaying an active node for the ActivatedNodeWidget
export const ActivatedNodeButton: React.FC<ActivatedNodeButtonProps> =
  (props: ActivatedNodeButtonProps) =>
  {
    return (
      <Button
        color = {props.isSelected ? "primary" : "default"}
        variant = {props.isSelected ? "shadow" : "ghost"}
        isDisabled={true}
        className="grow opacity-100"
      >
        {props.icon}
        {props.text}
      </Button>
    );
  }

