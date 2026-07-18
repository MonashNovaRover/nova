import { Button, ButtonProps, Tooltip, TooltipProps } from "@nextui-org/react";

interface TooltipButtonProps extends ButtonProps {
  tooltipContent: string;
  placement?: TooltipProps["placement"];
}

export const ToolTipButton = (props: TooltipButtonProps) => {
  return (
    <Tooltip
      className="dark text-foreground"
      content={props.tooltipContent}
      color={props.color} // Tooltip to be the same color as the button
      placement={props.placement}
    >
      <Button {...props}>{props.children}</Button>
    </Tooltip>
  );
};
