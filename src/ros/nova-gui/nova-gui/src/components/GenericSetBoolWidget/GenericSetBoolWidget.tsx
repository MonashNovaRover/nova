import React, {useEffect, useState} from "react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../ros/services/rosService.ts";
import {IRosStdSrvsSetBoolResponse} from "../../ros/rosTypes.ts";
import {Card, CardBody, CardHeader, CardProps, Switch} from "@nextui-org/react";

export interface GenericSetBoolWidgetProps extends CardProps {
  label?: string
  service: RosService
}

const GenericSetBoolWidget: React.FC<GenericSetBoolWidgetProps> = (props) => {
  const [isOn, setIsOn] = useState<boolean>(false);
  const bifrost = useBifrost({service: props.service});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const changeValue = (value: boolean) => {
    bifrost.callService({data: value}, {
      handleResponse: (response) => {
        const boolResponse = response as IRosStdSrvsSetBoolResponse;
        if (boolResponse?.success) {
          setIsOn(value);
        }
      }
    });
  }

  return <Card {...props}>
    <CardHeader className="justify-center">{props.label ?? props.service}</CardHeader>
    <CardBody className="items-center">
      <Switch size="lg" isSelected={isOn} onChange={() => changeValue(!isOn)}/>
    </CardBody>
  </Card>
}

export default GenericSetBoolWidget