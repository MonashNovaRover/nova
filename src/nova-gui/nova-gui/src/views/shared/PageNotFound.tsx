import { Button } from "@nextui-org/react";
import { useNavigate } from "react-router-dom";

const PageNotFoundView = () => {
  const navigate = useNavigate();

  return (
    <div className="flex flex-col items-center justify-center h-[calc(100vh-6rem)] text-center">
      <h1 className="text-6xl font-bold">404 Error!</h1>
      <p className="mt-2">Page not found :(</p>
      <Button className="mt-4" variant="ghost" onClick={() => navigate("/")}>
        Go to Home
      </Button>
    </div>
  );
}

export default PageNotFoundView