import {Button, ButtonProps, Spinner, SpinnerProps} from "@heroui/react";
import {FC, ReactNode} from "react";
import Overlay from "../Overlay/Overlay.tsx";

export interface SpinnerButtonProps extends Omit<ButtonProps, "children"> {
  children?: ReactNode;
  isLoading?: boolean;
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
  const { children, isLoading = false, spinner, spinnerProps, ...buttonProps } = props;

  const spinnerOverlay = (
    spinner ||
    <Spinner
      size={props.size == "lg" ? "md" : "sm"}
      color="current"
      className={"transition-opacity pointer-events-none ease-out " + (isLoading ? "" : "opacity-0")}
      {...spinnerProps}
    />
  )

  const button = (
    <Button
      fullWidth
      {...buttonProps}
      isDisabled={props.isDisabled || isLoading}
    >
      <div className={isLoading ? "blur-[1pt] transition-all ease-in" : "transition-all ease-in"}>
        {children}
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