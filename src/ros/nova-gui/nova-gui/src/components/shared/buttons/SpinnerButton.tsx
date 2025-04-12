import {Button, ButtonProps, Spinner, SpinnerProps} from "@nextui-org/react";
import {FC, ReactNode} from "react";
import Overlay from "../Overlay/Overlay.tsx";

export interface SpinnerButtonProps extends ButtonProps {
  // Custom defined spinner element (optional)
  spinner?: ReactNode
  // Props for the default spinner element
  spinnerProps?: SpinnerProps
}

/**
 * Button with alternative `isLoading` appearance, with centred spinner that fades in and out. It looks really nice.
 * @param props same as button props, but allows for the `spinner` or `spinnerProps` to be defined too
 * @constructor
 */
const SpinnerButton: FC<SpinnerButtonProps> = (props) => {

  const spinnerOverlay = (
    props.spinner ||
    <Spinner
      size={props.size == "lg" ? "md" : "sm"}
      color={!props.variant || props.variant === "solid" || props.variant === "shadow" ? "white" : props.color}
      className={"transition-opacity pointer-events-none ease-out " + (props.isLoading ? "" : "opacity-0")}
      {...props.spinnerProps}
    />
  )

  const button = (
    <Button
      fullWidth
      {...props}
      isDisabled={props.isDisabled || props.isLoading}
      isLoading={false}
    >
      <div className={props.isLoading ? "blur-[1pt] transition-all ease-in" : "transition-all ease-in"}>
        {props.children}
      </div>
    </Button>
  );

  return (
    <Overlay
      overlay={spinnerOverlay}
    >
      {button}
    </Overlay>
  );
}

export default SpinnerButton;