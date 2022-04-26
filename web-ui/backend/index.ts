import express, { Application, Request, Response } from 'express';
import cors from 'cors';
import { PrismaClient } from '@prisma/client';
import 'dotenv/config';
import httpStatusCodes from 'http-status-codes';
import { Application as App, Session, AppSession } from './interfaces';
import { PrismaClientKnownRequestError } from '@prisma/client/runtime';

if (process.env.NODE_ENV === 'production') console.log = () => {};

const prisma = new PrismaClient({
  datasources: {
    db: {
      url: `file:/var/lib/omnis/omnis.db`,
    },
  },
});

const app: Application = express();
const port = process.env.PORT;

const corsOptions: cors.CorsOptions = {
  origin: ['http://localhost:3000'],
};

app.use(cors(corsOptions));
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.get('/', async (req: Request, res: Response): Promise<Response> => {
  return res
    .status(httpStatusCodes.OK)
    .send({ message: 'Server operating normally.' });
});

app.get('/data', async (req: Request, res: Response): Promise<Response> => {
  if (!req.query.start || !req.query.end) {
    return res
      .status(httpStatusCodes.BAD_REQUEST)
      .send({ message: 'Start and End datetimes required!' });
  }

  const start = Math.floor(Date.parse(req.query.start.toString()) / 1000);
  const end = Math.floor(Date.parse(req.query.end.toString()) / 1000);

  if (isNaN(start) || isNaN(end)) {
    return res.status(httpStatusCodes.BAD_REQUEST).send({
      message: 'Parse error! Start and End must be date in ISO string format.',
    });
  }

  let data: AppSession[] = [];
  try {
    const app_session = await prisma.application.findMany({
      include: {
        Session: {
          where: {
            start: {
              gte: start,
              lt: end,
            },
          },
          orderBy: {
            start: 'asc',
          },
        },
      },
    });

    app_session.map((val) => {
      if (val.Session.length > 0) {
        const application: App = {
          id: val.id,
          name: val.name,
          colorHex: val.colorHex,
        };

        const sessions: Session[] = val.Session.map((s) => ({
          start: s.start,
          durationSec: s.durationSec,
          applicationId: s.applicationId,
          bytesTx: s.bytesTx,
          bytesRx: s.bytesRx,
          pktTx: s.pktTx,
          pktRx: s.pktRx,
          pktTotal: s.pktTx + s.pktRx,
          pktTcp: s.pktTcp,
          pktUdp: s.pktUdp,
        }));

        data.push({ application, sessions });
      }
    });
  } catch (e) {
    if (e instanceof PrismaClientKnownRequestError) {
      const errMsg = 'Error: ' + e.message + '\n' + e.meta;
      console.log(errMsg);
      return res
        .status(httpStatusCodes.INTERNAL_SERVER_ERROR)
        .send({ message: errMsg });
    }
  }

  return res.status(httpStatusCodes.OK).send({ data: data });
});

app.get(
  '/applications',
  async (req: Request, res: Response): Promise<Response> => {
    try {
      const apps = await prisma.application.findMany();
      return res.status(httpStatusCodes.OK).send({ data: apps });
    } catch (e) {
      if (e instanceof PrismaClientKnownRequestError) {
        const errMsg = 'Error: ' + e.message + '\n' + e.meta;
        console.log(errMsg);
        return res
          .status(httpStatusCodes.INTERNAL_SERVER_ERROR)
          .send({ message: errMsg });
      }
      return res
        .status(httpStatusCodes.INTERNAL_SERVER_ERROR)
        .send({ message: 'Error: ' + e });
    }
  }
);

app.put(
  '/applications',
  async (req: Request<{}, {}, App>, res: Response): Promise<Response> => {
    try {
      await prisma.application.update({
        where: {
          name: req.body.name,
        },
        data: {
          colorHex: req.body.colorHex,
        },
      });
    } catch (e) {
      if (e instanceof PrismaClientKnownRequestError) {
        const errMsg = 'Error: ' + e.message + '\n' + e.meta;
        console.log(errMsg);
        return res
          .status(httpStatusCodes.INTERNAL_SERVER_ERROR)
          .send({ message: errMsg });
      }
      return res
        .status(httpStatusCodes.INTERNAL_SERVER_ERROR)
        .send({ message: 'Error: ' + e });
    }

    return res
      .status(httpStatusCodes.OK)
      .send({ message: 'Application color changed successfully.' });
  }
);

try {
  app.listen(port, (): void => {
    console.log(`Listening at port ${port}`);
  });
} catch (error: any) {
  console.log(`Error occurred: ${error.message}`);
}
