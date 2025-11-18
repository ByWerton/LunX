// Firebase SDK'larından ihtiyacınız olan fonksiyonları içeri aktarın
import { initializeApp } from "firebase/app";
import { getAnalytics } from "firebase/analytics"; 
// Diğer Firebase ürünlerini kullanmak için (örn. Firestore, Auth) bunları da eklemelisiniz:
// import { getFirestore } from "firebase/firestore";
// import { getAuth } from "firebase/auth";

// Sizin Web uygulamanızın Firebase yapılandırması
const firebaseConfig = {
  apiKey: "AIzaSyD4Dh_WCxTSqMnJ8u0M4KqwHpejJrEoctk",
  authDomain: "lunxstudio.firebaseapp.com",
  databaseURL: "https://lunxstudio-default-rtdb.firebaseio.com",
  projectId: "lunxstudio",
  storageBucket: "lunxstudio.firebasestorage.app",
  messagingSenderId: "810358489202",
  appId: "1:810358489202:web:301dbd444bf4608f2782eb",
  measurementId: "G-Y36QNCE71R"
};

// Firebase'i Başlatma
const app = initializeApp(firebaseConfig);

// Firebase Analytics'i Başlatma
// Analytics'i kullanmak istemiyorsanız bu satırı silebilirsiniz.
const analytics = getAnalytics(app);

// Artık diğer Firebase hizmetlerini bu 'app' nesnesini kullanarak başlatabilirsiniz:
// const db = getFirestore(app);
// const auth = getAuth(app);

// İhtiyacınız olursa, diğer dosyalarda kullanmak üzere 'app' nesnesini dışa aktarabilirsiniz.
// export { app, analytics, db, auth };
