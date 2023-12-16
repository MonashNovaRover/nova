import { Badge, Button, ButtonProps, Kbd, Tooltip } from "@nextui-org/react"
import { IDriveModeDisplayData } from "./DriveModeDisplayData"
import { cloneElement } from "react"

type TooltipPlacement = "top-start" | "top" | "top-end" | "bottom-start" 
| "bottom" | "bottom-end" | "left-start" | "left" | "left-end" | "right-start" 
| "right" | "right-end";
type BadgePlacement = "top-left" | "top-right" | "bottom-left" | "bottom-right"

// Properties for the DriveModeButton component.
export interface IDriveModeButtonProps extends ButtonProps {
  driveModeData: IDriveModeDisplayData,
  driveModeActive: boolean,
  iconClassName?: string,
  tooltopPlacement?: TooltipPlacement,
  keybindPlacement?: BadgePlacement,
  hideTooltip?: boolean,
  wrapperClassName?: string,
}

// A button used for selecting a drive mode for the DriveWidget
export const DriveModeButton: React.FC<IDriveModeButtonProps> = 
  (props: IDriveModeButtonProps) => 
{
  // Apply additional styles in props.iconClassName to the icon element
  const icon = props.iconClassName === undefined ? props.driveModeData.icon : 
    cloneElement(props.driveModeData.icon, { className: 
      `${props.driveModeData.icon.props.className} ${props.iconClassName}`}
    )
  
  // The inner button element containing the drive mode's icon
  const innerButton = (
    <Button {...props}
      isIconOnly = { props.children === undefined }
      color = {props.driveModeActive ? "primary" : "default"} 
      variant = {props.driveModeActive ? "shadow" : "ghost"} 
    >
      {icon}
      {props.children}
    </Button>
  );

  // The inner button wrapped in a tooltip
  const tooltipButton = props.hideTooltip ? innerButton : (
    <Tooltip 
      content={props.driveModeData.name} 
      placement={props.tooltopPlacement}
      showArrow={true}
    > 
      {innerButton}
    </Tooltip>
  );

  // The result is wrapped in a badge with the drive mode's keybind
  return (
    <Badge
      isInvisible = {props.driveModeData.keybind === undefined}
      content = {<span>{props.driveModeData.keybind}</span>} 
      classNames={{"badge": `DriveModeKeybind text-foreground-600 bg-default-100
        rounded-small text-center text-small font-sans font-normal`}}
      placement={props.keybindPlacement}
    >
      {tooltipButton}
    </Badge>
  )
}

