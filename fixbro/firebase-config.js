const firebaseConfig = {
//isi
};
let app;
if (!firebase.apps.length) { 
  app = firebase.initializeApp(firebaseConfig);
} else {
  app = firebase.app(); 
}
const db = firebase.database(app); 
export { app, db };
