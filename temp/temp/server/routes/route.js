import express from "express"
import { getTempMoisture } from "../controllers/controller.js"

export const router=express.Router()

router.get("/",getTempMoisture)