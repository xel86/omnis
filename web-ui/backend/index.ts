import express, { Application, Request, Response } from 'express';
import cors from 'cors';
import { PrismaClient } from '@prisma/client';
import 'dotenv/config';
import httpStatusCodes from 'http-status-codes';
import * as os from 'os';
import * as fs from 'fs';
import * as ini from 'ini';
import { Application as App, Session, AppSession } from './interfaces';
import { PrismaClientKnownRequestError } from '@prisma/client/runtime';

if (process.env.NODE_ENV === 'production') console.log = () => {};

const config = ini.parse(
  fs.readFileSync(os.homedir + '/.config/omnis/config.ini', 'utf-8')
);
const prisma = new PrismaClient({
  datasources: {
    db: {
      url: `file:${os.homedir + config.database.url}`,
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

  const start = Date.parse(req.query.start.toString());
  const end = Date.parse(req.query.end.toString());

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

    data = app_session.map((val) => {
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

      return { application, sessions };
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

app.post(
  '/applications',
  async (req: Request<{}, {}, App[]>, res: Response): Promise<Response> => {
    for (const app of req.body) {
      if (!app.colorHex) app.colorHex = '';
      try {
        await prisma.application.create({ data: app });
      } catch (e) {
        if (e instanceof PrismaClientKnownRequestError) {
          const errMsg = 'Error: ' + e.message + '\n' + e.meta;
          console.log(errMsg);
          return res
            .status(httpStatusCodes.INTERNAL_SERVER_ERROR)
            .send({ message: errMsg });
        }
      }
    }

    return res
      .status(httpStatusCodes.OK)
      .send({ message: 'Applications inserted successfully.' });
  }
);

app.post(
  '/sessions',
  async (req: Request<{}, {}, Session[]>, res: Response): Promise<Response> => {
    for (const app of req.body) {
      try {
        await prisma.session.create({ data: app });
      } catch (e) {
        if (e instanceof PrismaClientKnownRequestError) {
          const errMsg = 'Error: ' + e.message + '\n' + e.meta;
          console.log(errMsg);
          return res
            .status(httpStatusCodes.INTERNAL_SERVER_ERROR)
            .send({ message: errMsg });
        }
      }
    }

    return res
      .status(httpStatusCodes.OK)
      .send({ message: 'Sessions inserted successfully.' });
  }
);

try {
  app.listen(port, (): void => {
    console.log(`Listening at port ${port}`);
  });
} catch (error: any) {
  console.log(`Error occurred: ${error.message}`);
}
