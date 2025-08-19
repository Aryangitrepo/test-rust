pub mod v1{

    use axum::{extract::WebSocketUpgrade, response::Response, routing::{any, get, MethodRouter}, Router};

    use crate::{ utils::{ socket::handle_socket}};
    
    pub fn router_new()->Router{
        Router::new()
        .route("/", hello())
        .route("/ws", any(ws))
        
    }

    fn hello()->MethodRouter{
        get(||async {
            "hello world"
        })
    }

    async fn ws(ws:WebSocketUpgrade)->Response{
        ws.on_upgrade(handle_socket)
    }
}
