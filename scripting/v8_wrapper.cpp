// v8_wrapper.cpp

#include "v8_wrapper.h"
#include "v8_utils.h"

#include <iostream>

using namespace std;
using namespace v8;

namespace mongo {

#define CONN_STRING (v8::String::New( "_conn" ))

#define DDD(x)

    Local<v8::Object> mongoToV8( const BSONObj& m , bool array ){
        Local<v8::Object> o;
        if ( array )
            o = v8::Array::New();
        else 
            o = v8::Object::New();
    
        mongo::BSONObj sub;
    
        for ( BSONObjIterator i(m); i.more(); ) {
            const BSONElement& f = i.next();
        
            Local<Value> v;
        
            switch ( f.type() ){

            case mongo::Code:
                cout << "warning, code saved in database just turned into string right now" << endl;
            case mongo::String: 
                o->Set( v8::String::New( f.fieldName() ) , v8::String::New( f.valuestr() ) );
                break;
            
            case mongo::jstOID: {
                v8::Function * idCons = getObjectIdCons();
                v8::Handle<v8::Value> argv[1];
                argv[0] = v8::String::New( f.__oid().str().c_str() );
                o->Set( v8::String::New( f.fieldName() ) , 
                        idCons->NewInstance( 1 , argv ) );
                break;
            }
            
            case mongo::NumberDouble:
            case mongo::NumberInt:
                o->Set( v8::String::New( f.fieldName() ) , v8::Number::New( f.number() ) );
                break;
            
            case mongo::Array:
            case mongo::Object:
                sub = f.embeddedObject();
                o->Set( v8::String::New( f.fieldName() ) , mongoToV8( sub , f.type() == mongo::Array ) );
                break;
            
            case mongo::Date:
                o->Set( v8::String::New( f.fieldName() ) , v8::Date::New( f.date() ) );
                break;

            case mongo::Bool:
                o->Set( v8::String::New( f.fieldName() ) , v8::Boolean::New( f.boolean() ) );
                break;
            
            case mongo::jstNULL:
                o->Set( v8::String::New( f.fieldName() ) , v8::Null() );
                break;
            
            case mongo::RegEx: {
                v8::Function * regex = getNamedCons( "RegExp" );
            
                v8::Handle<v8::Value> argv[2];
                argv[0] = v8::String::New( f.regex() );
                argv[1] = v8::String::New( f.regexFlags() );
            
                o->Set( v8::String::New( f.fieldName() ) , regex->NewInstance( 2 , argv ) );
                break;
            }
            
            case mongo::BinData: {
                Local<v8::Object> b = v8::Object::New();

                int len;
                f.binData( len );
            
                b->Set( v8::String::New( "subtype" ) , v8::Number::New( f.binDataType() ) );
                b->Set( v8::String::New( "length" ) , v8::Number::New( len ) );
            
                o->Set( v8::String::New( f.fieldName() ) , b );
                break;
            };
            
            case mongo::Timestamp: {
                Local<v8::Object> sub = v8::Object::New();            

                sub->Set( v8::String::New( "time" ) , v8::Date::New( f.timestampTime() ) );
                sub->Set( v8::String::New( "i" ) , v8::Number::New( f.timestampInc() ) );
            
                o->Set( v8::String::New( f.fieldName() ) , sub );
                break;
            }
            
            case mongo::MinKey:
                // TODO: make a special type
                o->Set( v8::String::New( f.fieldName() ) , v8::String::New( "MinKey" ) );
                break;

            case mongo::MaxKey:
                // TODO: make a special type
                o->Set( v8::String::New( f.fieldName() ) , v8::String::New( "MaxKey" ) );
                break;
            
            default:
                cout << "can't handle type: ";
                cout  << f.type() << " ";
                cout  << f.toString();
                cout  << endl;
                break;
            }
        
        }

        return o;
    }

    Handle<v8::Value> mongoToV8Element( const BSONElement &f ) {
        switch ( f.type() ){

        case mongo::Code:
            cout << "warning, code saved in database just turned into string right now" << endl;
        case mongo::String: 
            return v8::String::New( f.valuestr() );
            
        case mongo::jstOID: {
            v8::Function * idCons = getObjectIdCons();
            v8::Handle<v8::Value> argv[1];
            argv[0] = v8::String::New( f.__oid().str().c_str() );
            return idCons->NewInstance( 1 , argv );
        }
            
        case mongo::NumberDouble:
        case mongo::NumberInt:
            return v8::Number::New( f.number() );
            
        case mongo::Array:
        case mongo::Object:
            return mongoToV8( f.embeddedObject() , f.type() == mongo::Array );
            
        case mongo::Date:
            return v8::Date::New( f.date() );
            
        case mongo::Bool:
            return v8::Boolean::New( f.boolean() );

        case mongo::EOO:            
        case mongo::jstNULL:
            return v8::Null();
            
        case mongo::RegEx: {
            v8::Function * regex = getNamedCons( "RegExp" );
            
            v8::Handle<v8::Value> argv[2];
            argv[0] = v8::String::New( f.regex() );
            argv[1] = v8::String::New( f.regexFlags() );
            
            return regex->NewInstance( 2 , argv );
            break;
        }
            
        case mongo::BinData: {
            Local<v8::Object> b = v8::Object::New();
            
            int len;
            f.binData( len );
            
            b->Set( v8::String::New( "subtype" ) , v8::Number::New( f.binDataType() ) );
            b->Set( v8::String::New( "length" ) , v8::Number::New( len ) );
            
            return b;
        };
            
        case mongo::Timestamp: {
            Local<v8::Object> sub = v8::Object::New();            
            
            sub->Set( v8::String::New( "time" ) , v8::Date::New( f.timestampTime() ) );
            sub->Set( v8::String::New( "i" ) , v8::Number::New( f.timestampInc() ) );
            
            return sub;
        }
            
        case mongo::MinKey:
            // TODO: make a special type
            return v8::String::New( "MinKey" );
            
        case mongo::MaxKey:
            // TODO: make a special type
            return v8::String::New( "MaxKey" );
            
        case mongo::Undefined:
            return v8::Undefined();
            
        default:
            cout << "can't handle type: ";
			cout  << f.type() << " ";
			cout  << f.toString();
			cout  << endl;
            break;
        }    
        
        return v8::Undefined();
    }

    void v8ToMongoElement( BSONObjBuilder & b , v8::Handle<v8::String> name , const string sname , v8::Handle<v8::Value> value ){
        
        if ( value->IsString() ){
            if ( sname == "$where" )
                b.appendCode( sname.c_str() , toSTLString( value ).c_str() );
            else
                b.append( sname.c_str() , toSTLString( value ).c_str() );
            return;
        }
        
        if ( value->IsFunction() ){
            b.appendCode( sname.c_str() , toSTLString( value ).c_str() );
            return;
        }
    
        if ( value->IsNumber() ){
            b.append( sname.c_str() , value->ToNumber()->Value() );
            return;
        }
    
        if ( value->IsArray() ){
            BSONObj sub = v8ToMongo( value->ToObject() );
            b.appendArray( sname.c_str() , sub );
            return;
        }
    
        if ( value->IsDate() ){
            b.appendDate( sname.c_str() , (unsigned long long )(v8::Date::Cast( *value )->NumberValue()) );
            return;
        }
    
        if ( value->IsObject() ){
            string s = toSTLString( value );
            if ( s.size() && s[0] == '/' ){
                s = s.substr( 1 );
                string r = s.substr( 0 , s.find( "/" ) );
                string o = s.substr( s.find( "/" ) + 1 );
                b.appendRegex( sname.c_str() , r.c_str() , o.c_str() );
            }
            else if ( value->ToObject()->GetPrototype()->IsObject() &&
                      value->ToObject()->GetPrototype()->ToObject()->HasRealNamedProperty( v8::String::New( "isObjectId" ) ) ){
                OID oid;
                oid.init( toSTLString( value ) );
                b.appendOID( sname.c_str() , &oid );
            }
            else {
                BSONObj sub = v8ToMongo( value->ToObject() );
                b.append( sname.c_str() , sub );
            }
            return;
        }
    
        if ( value->IsBoolean() ){
            b.appendBool( sname.c_str() , value->ToBoolean()->Value() );
            return;
        }
    
        else if ( value->IsUndefined() ){
            return;
        }
    
        else if ( value->IsNull() ){
            b.appendNull( sname.c_str() );
            return;
        }

        cout << "don't know how to covert to mongo field [" << name << "]\t" << value << endl;
    }

    BSONObj v8ToMongo( v8::Handle<v8::Object> o ){
        BSONObjBuilder b;

        v8::Handle<v8::String> idName = v8::String::New( "_id" );
        if ( o->HasRealNamedProperty( idName ) ){
            v8ToMongoElement( b , idName , "_id" , o->Get( idName ) );
        }
    
        Local<v8::Array> names = o->GetPropertyNames();
        for ( unsigned int i=0; i<names->Length(); i++ ){
            v8::Local<v8::String> name = names->Get(v8::Integer::New(i) )->ToString();

            if ( o->GetPrototype()->IsObject() &&
                 o->GetPrototype()->ToObject()->HasRealNamedProperty( name ) )
                continue;
        
            v8::Local<v8::Value> value = o->Get( name );
        
            const string sname = toSTLString( name );
            if ( sname == "_id" )
                continue;

            v8ToMongoElement( b , name , sname , value );
        }
        return b.obj();
    }

    v8::Function * getNamedCons( const char * name ){
        return v8::Function::Cast( *(v8::Context::GetCurrent()->Global()->Get( v8::String::New( name ) ) ) );
    }

    v8::Function * getObjectIdCons(){
        return getNamedCons( "ObjectId" );
    }

}
