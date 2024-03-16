import { Button, ButtonProps, Tooltip } from "@nextui-org/react";

interface TooltipButtonProps extends ButtonProps {
  tooltipContent: string;
}

export const ToolTipButton = (props: TooltipButtonProps) => {
  return (
    <Tooltip
      className="dark text-foreground"
      content={props.tooltipContent}
      color={props.color} // Tooltip to be the same color as the button
    >
      <Button {...props}>{props.children}</Button>
    </Tooltip>
  );
};
