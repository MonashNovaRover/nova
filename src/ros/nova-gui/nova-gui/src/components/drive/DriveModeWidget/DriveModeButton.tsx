import { Badge, Button, ButtonProps, Tooltip } from "@nextui-org/react"
import { IDriveModeDisplayData } from "./DriveModeDisplayData.tsx"
import { cloneElement } from "react"
import { OverlayPlacement } from "@nextui-org/aria-utils"

type BadgePlacement = "top-right" | "top-left" |  "bottom-right" | "bottom-left"

// Properties for the DriveModeButton component.
export interface IDriveModeButtonProps extends ButtonProps {
  driveModeData: IDriveModeDisplayData,
  driveModeActive: boolean,
  iconClassName?: string,
  tooltipPlacement?: OverlayPlacement,
  keybindPlacement?: BadgePlacement,
  hideTooltip?: boolean,
  hideKeybind?: boolean,
  wrapperClassName?: string,
}

// A button used for selecting a drive mode for the DriveModeWidget
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
    <Button  {...props} aria-label={`${props.driveModeData.name} Mode`}
      isIconOnly = { props.children === undefined || props.isIconOnly }
      color = {props.driveModeActive ? "primary" : "default"} 
      variant = {props.driveModeActive ? "shadow" : "ghost"}
            className={`${props.className} DriveModeButton`}
    >
      <div className="DriveModeButtonContainer">
        <div className="DriveModeButtonIconContainer">{icon}</div>
        {props.children}
      </div>

    </Button>
  );

  // The inner button wrapped in a tooltip
  const tooltipButton = props.hideTooltip ? innerButton : (
    <Tooltip 
      content={props.driveModeData.name}
      placement={props.tooltipPlacement ?? "top-end"}
      showArrow={true}
      className={props.className}
    > 
      {innerButton}
    </Tooltip>
  );

  // Add a badge to the button for any keybind if not props.hideKeybind
  const badgedButton = (
      <Badge
        isInvisible={props.driveModeData.keybind === undefined || (props.hideKeybind ?? false)}
        content = {<span>{props.driveModeData.keybind}</span>}
        classNames={{"badge": `DriveModeKeybind text-foreground-600 bg-default-100
          rounded-small text-center text-small font-sans font-normal`}}
        placement={props.keybindPlacement ?? "top-right"}
      >
        {tooltipButton}
      </Badge>
  )

  // The result is wrapped in a badge with the drive mode's keybind
  return badgedButton;
}

