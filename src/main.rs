

use crate::utils::{ route};

mod utils;

#[tokio::main]
async fn main() {
    let app= route::v1::router_new();
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    println!("server started");
    axum::serve(listener, app).await.unwrap();
}
