import express, { Application, Request, Response } from "express";
import { PrismaClient } from "@prisma/client";
import "dotenv/config";
import httpStatusCodes from "http-status-codes";

if (process.env.NODE_ENV === "production") console.log = () => {};

const app: Application = express();
const prisma = new PrismaClient();
const port = process.env.PORT;

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.get("/", async (req: Request, res: Response): Promise<Response> => {
  return res
    .status(httpStatusCodes.OK)
    .send({ message: "Server operating normally." });
});

app.get("/data", async (req: Request, res: Response): Promise<Response> => {
  if (!req.query.start || !req.query.end) {
    return res
      .status(httpStatusCodes.BAD_REQUEST)
      .send({ message: "Start and End datetimes required!" });
  }

  const start = Date.parse(req.query.start.toString());
  const end = Date.parse(req.query.end.toString());

  if (isNaN(start) || isNaN(end)) {
    return res.status(httpStatusCodes.BAD_REQUEST).send({
      message: "Parse error! Start and End must be date in ISO string format.",
    });
  }

  const app_session = await prisma.application.findMany({
    include: {
      Session: {
        where: {
          start: {
            gte: new Date(start),
            lt: new Date(end),
          },
        },
      },
    },
  });

  return res.status(httpStatusCodes.OK).send({ data: app_session });
});

try {
  app.listen(port, (): void => {
    console.log(`Listening at port ${port}`);
  });
} catch (error: any) {
  console.log(`Error occurred: ${error.message}`);
}
