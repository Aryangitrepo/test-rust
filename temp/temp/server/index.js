import express from "express"
import { router } from "./routes/route.js"

const server=express()

server.use(router)

server.listen(8000,()=>{
    console.log("server started")
})