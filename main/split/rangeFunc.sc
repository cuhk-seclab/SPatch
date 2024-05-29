import overflowdb.Node
import scala.collection.mutable._
import scala.io.Source
import scala.util.control.Breaks._
import java.io.File
import java.io.FileWriter
import org.json4s.native.Json
import org.json4s.DefaultFormats

@main def exec(cpgFile: String, program: String, genDir: String, upFile: String) = {
   loadCpg(cpgFile)
   val methods=cpg.method.name.l
   val map: Map[String, List[Int]] = Map[String, List[Int]]()
   
   methods.foreach{ mtd =>
      //we only want user-defined functions in upFile
      if(mtd.matches("^[a-zA-Z_][a-zA-Z0-9_]*$")) {
         val ls=cpg.method(mtd).filter(_.filename==upFile).lineNumber.l
         val le=cpg.method(mtd).filter(_.filename==upFile).lineNumberEnd.l
         if(!ls.isEmpty && !le.isEmpty) {
            val lineStart=ls.head.toInt
            val lineEnd=le.head.toInt
            val loc: List[Int]=List(lineStart, lineEnd)
            var k=mtd
            map+=(k->loc)
         }
      }
   }

   val js=Json(DefaultFormats).write(map)
   var idx=cpgFile.lastIndexOf('/')
   var fileName=cpgFile.substring(idx+1)
   val file = new File(genDir+"+"+fileName)
   val fw = new FileWriter(file)
   fw.write(js)
   fw.close()
}