import {Button, Card, CardBody, CardHeader} from "@nextui-org/react";
import {useCallback, useState} from "react";
import SpinnerButton from "../../../components/shared/buttons/SpinnerButton.tsx";

export default function TestOverlayView () {
  const [isLoading, setIsLoading] = useState<boolean>(false);

  const load = useCallback(() => {
    setIsLoading(true);

    const timeoutId = setTimeout(() => {
      setIsLoading(false);
    }, 1000)

    return () => {
      clearTimeout(timeoutId)
    }
  }, []);

  return (
    <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">

      <Card>
        <CardHeader>Spinner buttons</CardHeader>

        <CardBody className="flex flex-col gap-3">
          <Button>
            Hello
          </Button>
          <Button isLoading={isLoading} onPress={load}>
            Hello
          </Button>
          <SpinnerButton isLoading={isLoading} onPress={load} size="lg">
            Hello
          </SpinnerButton>
          <SpinnerButton isLoading={isLoading} onPress={load}>
            Hello
          </SpinnerButton>
          <SpinnerButton isLoading={isLoading} onPress={load} size="sm">
            Hello
          </SpinnerButton>

          <SpinnerButton isLoading={isLoading} onPress={load} color="primary">Hello</SpinnerButton>
          <SpinnerButton isLoading={isLoading} onPress={load} color="primary" variant="ghost">Hello</SpinnerButton>
          <SpinnerButton isLoading={isLoading} onPress={load} color="primary" variant="light">Hello</SpinnerButton>
          <SpinnerButton isLoading={isLoading} onPress={load} color="primary" variant="faded">Hello</SpinnerButton>
          <SpinnerButton isLoading={isLoading} onPress={load} color="primary" variant="flat">Hello</SpinnerButton>
        </CardBody>
      </Card>


    </div>
  );
}
