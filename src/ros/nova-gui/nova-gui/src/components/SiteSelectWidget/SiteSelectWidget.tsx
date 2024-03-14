import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";

export const siteFilenames = [
  "site1",
  "site2",
  "site3",
  "site4"
]

export interface SiteSelectWidgetProps extends CardProps {
  onValueChanged: (value: string) => void
}

const SiteSelectWidget: React.FC<SiteSelectWidgetProps> = ({onValueChanged, ...cardProps}) => {



  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        Site Select
      </CardHeader>
      <CardBody>

        <SegmentedPicker fullWidth onIndexChange={(i) => onValueChanged?.(siteFilenames[i])}>
          <>Site 1</>
          <>Site 2</>
          <>Site 3</>
          <>Site 4</>
        </SegmentedPicker>

      </CardBody>
    </Card>
  );
};

export default SiteSelectWidget;