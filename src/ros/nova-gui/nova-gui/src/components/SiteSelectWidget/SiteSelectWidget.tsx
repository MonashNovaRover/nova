import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";

const SiteSelectWidget: React.FC<CardProps> = (props) => {

  return (
    <Card {...props}>
      <CardHeader className="pb-0">
        Site Select
      </CardHeader>
      <CardBody>

        <SegmentedPicker fullWidth>
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