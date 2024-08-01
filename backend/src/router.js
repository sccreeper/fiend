import { Router } from 'itty-router';
import changeToWav from './routes/changeToWav';

const router = Router();

router.post("/api/changePcmToWav", (request) => changeToWav(request))

router.get("/", () => Response.redirect("https://www.oscarcp.net", 303))

// 404 for anything else
router.all('*', () => new Response('Not Found.', { status: 404 }));

export default router;
