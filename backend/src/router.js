import { Router } from 'itty-router';
import routeChangeToWav from './routes/changeToWav';
import routeAskQuestion from './routes/askQuestion';

const router = Router();

router.post("/api/changeToWav", (request, ctx) => routeChangeToWav(request, ctx))
router.post("/api/askQuestion", (request, ctx) => routeAskQuestion(request, ctx))

router.get("/", () => Response.redirect("https://www.oscarcp.net", 303))

// 404 for anything else
router.all('*', () => new Response('Not Found.', { status: 404 }));

export default router;
